#ifndef _NASAL_H
#define _NASAL_H
#ifdef __cplusplus
extern "C" {
#endif

#define TARGET_QNX
#ifdef TARGET_QNX
#define EXPORT
#define CALL
#else
#define EXPORT						__declspec(dllexport)
#define CALL						_cdecl
#endif

#ifndef BYTE_ORDER

# if (BSD >= 199103)
#  include <machine/endian.h>
# elif defined(__CYGWIN__) || defined(__MINGW32__)
#  include <sys/param.h>
# elif defined(linux)
#  include <endian.h>
# else
#  ifndef LITTLE_ENDIAN
#   define LITTLE_ENDIAN   1234    /* LSB first: i386, vax */
#  endif
#  ifndef BIG_ENDIAN
#   define BIG_ENDIAN      4321    /* MSB first: 68000, ibm, net */
#  endif

#  if defined(ultrix) || defined(__alpha__) || defined(__alpha) ||  \
      defined(__i386__) || defined(__i486__) || defined(_X86_) ||   \
      defined(sun386) || defined(TARGET_QNX)
#   define BYTE_ORDER LITTLE_ENDIAN
#  else
#   define BYTE_ORDER BIG_ENDIAN
#  endif
# endif /* BSD */
#endif /* BYTE_ORDER */

#if BYTE_ORDER == BIG_ENDIAN
# include <limits.h>
# if (LONG_MAX == 2147483647)
#  define NASAL_BIG_ENDIAN_32_BIT 1
# endif
#endif

// This is a nasal "reference".  They are always copied by value, and
// contain either a pointer to a garbage-collectable nasal object
// (string, vector, hash) or a floating point number.  Keeping the
// number here is an optimization to prevent the generation of
// zillions of tiny "number" object that have to be collected.  Note
// sneaky hack: on little endian systems, placing reftag after ptr and
// putting 1's in the top 13 (except the sign bit) bits makes the
// double value a NaN, and thus unmistakable (no actual number can
// appear as a reference, and vice versa).  Swap the structure order
// on 32 bit big-endian systems.  On 64 bit sytems of either
// endianness, reftag and the double won't be coincident anyway.
#define NASAL_REFTAG 0x7ff56789 // == 2,146,789,257 decimal
typedef union {
    double num;
    struct {
#ifdef NASAL_BIG_ENDIAN_32_BIT
        int reftag; // Big-endian systems need this here!
#endif
        union {
            struct naObj* obj;
            struct naStr* str;
            struct naVec* vec;
            struct naHash* hash;
            struct naCode* code;
            struct naFunc* func;
            struct naCCode* ccode;
            struct naGhost* ghost;
        } ptr;
#ifndef NASAL_BIG_ENDIAN_32_BIT
        int reftag; // Little-endian and 64 bit systems need this here!
#endif
    } ref;
} naRef;

typedef struct Context* naContext;
    
// The function signature for an extension function:
typedef naRef (*naCFunction)(naContext ctx, naRef me, int argc, naRef* args);

// All Nasal code runs under the watch of a naContext:
EXPORT naContext CALL naNewContext();
EXPORT void CALL naFreeContext(naContext c);

// Save this object in the context, preventing it (and objects
// referenced by it) from being garbage collected.
EXPORT void CALL naSave(naContext ctx, naRef obj);

// Similar, but the object is automatically released when the
// context next runs native bytecode.  Useful for saving off C-space
// temporaries to protect them before passing back into a naCall.
EXPORT void CALL naTempSave(naContext c, naRef r);

// Parse a buffer in memory into a code object.
EXPORT naRef CALL naParseCode(naContext c, naRef srcFile, int firstLine,
                  char* buf, int len, int* errLine);

// Binds a bare code object (as returned from naParseCode) with a
// closure object (a hash) to act as the outer scope / namespace.
// FIXME: this API is weak.  It should expose the recursive nature of
// closures, and allow for extracting the closure and namespace
// information from function objects.
EXPORT naRef CALL naBindFunction(naContext ctx, naRef code, naRef closure);

// Similar, but it binds to the current context's closure (i.e. the
// namespace at the top of the current call stack).
EXPORT naRef CALL naBindToContext(naContext ctx, naRef code);

// Call a code or function object with the specifed arguments "on" the
// specified object and using the specified hash for the local
// variables.  Any of args, obj or locals may be nil.
EXPORT naRef CALL naCall(naContext ctx, naRef func, int argc, naRef* args, naRef obj, naRef locals);

// Throw an error from the current call stack.  This function makes a
// longjmp call to a handler in naCall() and DOES NOT RETURN.  It is
// intended for use in library code that cannot otherwise report an
// error via the return value, and MUST be used carefully.  If in
// doubt, return naNil() as your error condition.
EXPORT void CALL naRuntimeError(naContext ctx, char* msg);

// Call a method on an object (NOTE: func is a function binding, *not*
// a code object as returned from naParseCode).
EXPORT naRef CALL naMethod(naContext ctx, naRef func, naRef object);

// Returns a hash containing functions from the Nasal standard library
// Useful for passing as a namespace to an initial function call
EXPORT naRef CALL naStdLib(naContext c);

// Ditto, for other core libraries
EXPORT naRef CALL naMathLib(naContext c);
EXPORT naRef CALL naBitsLib(naContext c);
EXPORT naRef CALL naIOLib(naContext c);
EXPORT naRef CALL naRegexLib(naContext c);
EXPORT naRef CALL naUnixLib(naContext c);

// Current line number & error message
EXPORT int		CALL naStackDepth(naContext ctx);
EXPORT int		CALL naGetLine(naContext ctx, int frame);
EXPORT naRef	CALL naGetSourceFile(naContext ctx, int frame);
EXPORT char*	CALL naGetError(naContext ctx);

// Type predicates
EXPORT int CALL naIsNil(naRef r);
EXPORT int CALL naIsNum(naRef r);
EXPORT int CALL naIsString(naRef r);
EXPORT int CALL naIsScalar(naRef r);
EXPORT int CALL naIsVector(naRef r);
EXPORT int CALL naIsHash(naRef r);
EXPORT int CALL naIsCode(naRef r);
EXPORT int CALL naIsFunc(naRef r);
EXPORT int CALL naIsCCode(naRef r);

// Allocators/generators:
EXPORT naRef CALL naNil();
EXPORT naRef CALL naNum(double num);
EXPORT naRef CALL naNewString(naContext c);
EXPORT naRef CALL naNewVector(naContext c);
EXPORT naRef CALL naNewHash(naContext c);
EXPORT naRef CALL naNewFunc(naContext c, naRef code);
EXPORT naRef CALL naNewCCode(naContext c, naCFunction fptr);

// Some useful conversion/comparison routines
EXPORT int		CALL naEqual(naRef a, naRef b);
EXPORT int		CALL naStrEqual(naRef a, naRef b);
EXPORT int		CALL naTrue(naRef b);
EXPORT naRef	CALL naNumValue(naRef n);
EXPORT naRef	CALL naStringValue(naContext c, naRef n);

// String utilities:
EXPORT int		CALL naStr_len(naRef s);
EXPORT char*	CALL naStr_data(naRef s);
EXPORT naRef	CALL naStr_fromdata(naRef dst, char* data, int len);
EXPORT naRef	CALL naStr_concat(naRef dest, naRef s1, naRef s2);
EXPORT naRef	CALL naStr_substr(naRef dest, naRef str, int start, int len);
EXPORT naRef	CALL naInternSymbol(naRef sym);

// Vector utilities:
EXPORT int		CALL naVec_size(naRef v);
EXPORT naRef	CALL naVec_get(naRef v, int i);
EXPORT void		CALL naVec_set(naRef vec, int i, naRef o);
EXPORT int		CALL naVec_append(naRef vec, naRef o);
EXPORT naRef	CALL naVec_removelast(naRef vec);
EXPORT void		CALL naVec_setsize(naRef vec, int sz);

// Hash utilities:
EXPORT int		CALL naHash_size(naRef h);
EXPORT int		CALL naHash_get(naRef hash, naRef key, naRef* out);
EXPORT naRef	CALL naHash_cget(naRef hash, char* key);
EXPORT void		CALL naHash_set(naRef hash, naRef key, naRef val);
EXPORT void		CALL naHash_cset(naRef hash, char* key, naRef val);
EXPORT void		CALL naHash_delete(naRef hash, naRef key);
EXPORT void		CALL naHash_keys(naRef dst, naRef hash);

// Ghost utilities:
typedef struct naGhostType {
    void (*destroy)(void* ghost);
} naGhostType;
EXPORT naRef        CALL naNewGhost(naContext c, naGhostType* t, void* ghost);
EXPORT naGhostType* CALL naGhost_type(naRef ghost);
EXPORT void*        CALL naGhost_ptr(naRef ghost);
EXPORT int          CALL naIsGhost(naRef r);

// Acquires a "modification lock" on a context, allowing the C code to
// modify Nasal data without fear that such data may be "lost" by the
// garbage collector (the C stack is not examined in GC!).  This
// disallows garbage collection until the current thread can be
// blocked.  The lock should be acquired whenever modifications to
// Nasal objects are made.  It need not be acquired when only read
// access is needed.  It MUST NOT be acquired by naCFunction's, as
// those are called with the lock already held; acquiring two locks
// for the same thread will cause a deadlock when the GC is invoked.
// It should be UNLOCKED by naCFunction's when they are about to do
// any long term non-nasal processing and/or blocking I/O.
EXPORT void CALL naModLock();
EXPORT void CALL naModUnlock();

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _NASAL_H
