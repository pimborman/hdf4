/* @(#)xdr.h    2.2 88/07/29 4.0 RPCSRC */
/*      @(#)xdr.h 1.19 87/04/22 SMI      */
/*
 * xdr.h, External Data Representation Serialization Routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef __XDR_HEADER__
#define __XDR_HEADER__

#include "H4api_adpt.h"

#include <stdio.h>

/*
 * XDR provides a conventional way for converting between C data
 * types and an external bit-string representation.  Library supplied
 * routines provide for the conversion on built-in C data types.  These
 * routines and utility routines defined here are used to help implement
 * a type encode/decode routine for each user-defined type.
 *
 * Each data type provides a single procedure which takes two arguments:
 *
 *    bool_t
 *    xdrproc(xdrs, argresp)
 *        XDR *xdrs;
 *        <type> *argresp;
 *
 * xdrs is an instance of a XDR handle, to which or from which the data
 * type is to be converted.  argresp is a pointer to the structure to be
 * converted.  The XDR handle contains an operation field which indicates
 * which of the operations (ENCODE, DECODE * or FREE) is to be performed.
 *
 * XDR_DECODE may allocate space if the pointer argresp is null.  This
 * data can be freed with the XDR_FREE operation.
 *
 * We write only one procedure per data type to make it easy
 * to keep the encode and decode procedures for a data type consistent.
 * In many cases the same code performs all operations on a user defined type,
 * because all the hard work is done in the component type routines.
 * decode as a series of calls on the nested data types.
 */

/*
 * Xdr operations.  XDR_ENCODE causes the type to be encoded into the
 * stream.  XDR_DECODE causes the type to be extracted from the stream.
 * XDR_FREE can be used to release the space allocated by an XDR_DECODE
 * request.
 */
enum xdr_op {
    XDR_ENCODE=0,
    XDR_DECODE=1,
    XDR_FREE=2
};

/*
 * This is the number of bytes per unit of external data.
 */
#define BYTES_PER_XDR_UNIT    (4)
#define RNDUP(x)  ((((x) + BYTES_PER_XDR_UNIT - 1) / BYTES_PER_XDR_UNIT) \
            * BYTES_PER_XDR_UNIT)

/*
 * The XDR handle.
 * Contains operation which is being applied to the stream,
 * an operations vector for the paticular implementation (e.g. see xdr_mem.c),
 * and two private fields for the use of the particular impelementation.
 */
typedef struct {
    enum xdr_op    x_op;        /* operation; fast additional param */
    struct xdr_ops {
        /* Get/put "long" (ie. 32 bit quantity). */
        bool_t    (*x_getlong)  (/*XDR *, int32_t * */);
        bool_t    (*x_putlong)  (/*XDR *, int32_t * */);
        /* Get/put bytes. */
        bool_t    (*x_getbytes) (/*XDR *, void *, size_t */);
        bool_t    (*x_putbytes) (/*XDR *, void *, size_t */);
        /* Get or seek within the stream (offsets from beginning of stream). */
        off_t     (*x_getpostn) (/*XDR * */);
        bool_t    (*x_setpostn) (/*XDR *, off_t */);
        /* Returns a pointer to the next n bytes in the stream. */
        int32_t *    (*x_inline)   (/*XDR *, size_t */);
        /* Free the stream. */
        void      (*x_destroy)  (/*XDR * */);
    } *x_ops;
    caddr_t     x_public;    /* users' data */
    caddr_t     x_private;   /* pointer to private data */
    caddr_t     x_base;      /* private used for position info */
    int         x_handy;     /* extra private word */
} XDR;

/*
 * A xdrproc_t exists for each data type which is to be encoded or decoded.
 *
 * The second argument to the xdrproc_t is a pointer to an opaque pointer.
 * The opaque pointer generally points to a structure of the data type
 * to be decoded.  If this pointer is 0, then the type routines should
 * allocate dynamic storage of the appropriate size and return it.
 * bool_t    (*xdrproc_t)(XDR *, caddr_t *);
 *
 * XXX can't actually prototype it, because some take three args!!!
 */
typedef    bool_t (*xdrproc_t)(XDR *, ...);

/* Define wrapper functions around the x_ops. */
static inline bool_t
xdr_getlong (XDR *xdrs, int32_t *v)
{
  return xdrs->x_ops->x_getlong (xdrs, v);
}
static inline bool_t
xdr_putlong (XDR *xdrs, int32_t *v)
{
  return xdrs->x_ops->x_putlong (xdrs, v);
}
static inline bool_t
xdr_getbytes (XDR *xdrs, void *p, size_t len)
{
  return xdrs->x_ops->x_getbytes (xdrs, p, len);
}
static inline bool_t
xdr_putbytes (XDR *xdrs, void *p, size_t len)
{
  return xdrs->x_ops->x_putbytes (xdrs, p, len);
}
static inline off_t
xdr_getpos (XDR *xdrs)
{
  return xdrs->x_ops->x_getpostn (xdrs);
}
static inline bool_t
xdr_setpos (XDR *xdrs, off_t v)
{
  return xdrs->x_ops->x_setpostn (xdrs, v);
}
static inline int32_t*
xdr_inline (XDR *xdrs, size_t len)
{
  return xdrs->x_ops->x_inline (xdrs, len);
}
static inline void
xdr_destroy (XDR *xdrs)
{
  return xdrs->x_ops->x_destroy (xdrs);
}


/* For compatibility with Sun XDR. */
#define XDR_GETLONG  xdr_getlong
#define XDR_PUTLONG  xdr_putlong
#define XDR_GETBYTES xdr_getbytes
#define XDR_PUTBYTES xdr_putbytes
#define XDR_GETPOS   xdr_getpos
#define XDR_SETPOS   xdr_setpos
#define XDR_INLINE   xdr_inline
#define XDR_DESTROY  xdr_destroy

/* Also seen in the wild ... */
#define XDR_GETINT32 xdr_getlong
#define XDR_PUTINT32 xdr_putlong

/*
 * Support struct for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * a entry with a null procedure pointer.  The xdr_union routine gets
 * the discriminant value and then searches the array of structures
 * for a matching value.  If a match is found the associated xdr routine
 * is called to handle that part of the union.  If there is
 * no match, then a default routine may be called.
 * If there is no match and no default routine it is an error.
 */
#define NULL_xdrproc_t ((xdrproc_t)0)
struct xdr_discrim {
    int       value;
    xdrproc_t proc;
};

/*
 * In-line routines for fast encode/decode of primitve data types.
 * Caveat emptor: these use single memory cycles to get the
 * data from the underlying buffer, and will fail to operate
 * properly if the data is not aligned.  The standard way to use these
 * is to say:
 *    if ((buf = XDR_INLINE(xdrs, count)) == NULL)
 *        return (FALSE);
 *    <<< macro calls >>>
 * where ``count'' is the number of bytes of data occupied
 * by the primitive data types.
 *
 * N.B. and frozen for all time: each data type here uses 4 bytes
 * of external representation.
 */

#define IXDR_GET_LONG(buf) ((int32_t) ntohl (*((int32_t *)(buf))++))
#define IXDR_GET_BOOL(buf) ((bool_t) IXDR_GET_LONG ((buf)))
#define IXDR_GET_ENUM(buf,type) ((type) IXDR_GET_LONG ((buf)))
#define IXDR_GET_U_LONG(buf) ((uint32_t) IXDR_GET_LONG ((buf)))
#define IXDR_GET_SHORT(buf) ((int16_t) IXDR_GET_LONG ((buf)))
#define IXDR_GET_U_SHORT(buf) ((uint16_t) IXDR_GET_LONG ((buf)))
#define IXDR_GET_INT32 IXDR_GET_LONG

#define IXDR_PUT_LONG(buf,v) ((*((int32_t *)(buf))++) = htonl ((v)))
#define IXDR_PUT_BOOL(buf,v) IXDR_PUT_LONG((buf), (int32_t) (v))
#define IXDR_PUT_ENUM(buf,v) IXDR_PUT_LONG((buf), (int32_t) (v))
#define IXDR_PUT_U_LONG(buf,v) IXDR_PUT_LONG((buf), (int32_t) (v))
#define IXDR_PUT_SHORT(buf,v) IXDR_PUT_LONG((buf), (int32_t) (v))
#define IXDR_PUT_U_SHORT(buf,v) IXDR_PUT_LONG((buf), (int32_t) (v))
#define IXDR_PUT_INT32 IXDR_PUT_LONG

/* Some very common aliases for the basic integer functions. */
#define xdr_int xdr_int32_t
#define xdr_u_int xdr_uint32_t
#define xdr_long xdr_int32_t
#define xdr_u_long xdr_uint32_t

#define xdr_short xdr_int16_t
#define xdr_u_short xdr_uint16_t

  /* NB: In PortableXDR, char is ALWAYS treated as signed octet. */
#define xdr_char xdr_int8_t
#define xdr_u_char xdr_uint8_t

/*
#define xdr_hyper xdr_int64_t
#define xdr_u_hyper xdr_uint64_t
#define xdr_quad xdr_int64_t
#define xdr_u_quad xdr_uint64_t
*/

/*
 * These are the "generic" xdr routines.
 */
#ifdef __cplusplus
extern "C" {
#endif

XDRLIBAPI void      xdr_free (xdrproc_t, char *);
XDRLIBAPI bool_t    xdr_void(void);
XDRLIBAPI bool_t    xdr_bool(XDR *, bool_t *);
XDRLIBAPI bool_t    xdr_enum(XDR *, enum_t *);
XDRLIBAPI bool_t    xdr_array(XDR *, char **, u_int *, u_int, u_int, xdrproc_t);
XDRLIBAPI bool_t    xdr_bytes(XDR *, char **, u_int *, u_int);
XDRLIBAPI bool_t    xdr_opaque(XDR *, char *, u_int);
XDRLIBAPI bool_t    xdr_string(XDR *, char **, u_int);
XDRLIBAPI bool_t    xdr_union(XDR *, enum_t *, char *, const struct xdr_discrim *, xdrproc_t);
XDRLIBAPI bool_t    xdr_vector(XDR *, char *, u_int, u_int, xdrproc_t);
XDRLIBAPI bool_t    xdr_float(XDR *, float *);
XDRLIBAPI bool_t    xdr_double(XDR *, double *);
XDRLIBAPI bool_t    xdr_reference(XDR *, char **, u_int, xdrproc_t);
XDRLIBAPI bool_t    xdr_pointer(XDR *, char **, u_int, xdrproc_t);
XDRLIBAPI bool_t    xdr_wrapstring(XDR *, char **);
XDRLIBAPI bool_t    xdr_uint64_t (XDR *xdrs, uint64_t *uip);
XDRLIBAPI bool_t    xdr_int64_t (XDR *xdrs, int64_t *uip);

/* XDR 64bit integers */
XDRLIBAPI bool_t    xdr_int64_t (XDR *xdrs, int64_t *ip);

/* XDR 64bit unsigned integers */
XDRLIBAPI bool_t    xdr_uint64_t (XDR *xdrs, uint64_t *uip);

/* XDR 32bit integers */
XDRLIBAPI bool_t    xdr_int32_t (XDR *xdrs, int32_t *lp);

/* XDR 32bit unsigned integers */
XDRLIBAPI bool_t    xdr_uint32_t (XDR *xdrs, uint32_t *ulp);

/* XDR 16bit integers */
XDRLIBAPI bool_t    xdr_int16_t (XDR *xdrs, int16_t *ip);

/* XDR 16bit unsigned integers */
XDRLIBAPI bool_t    xdr_uint16_t (XDR *xdrs, uint16_t *uip);

/* XDR 8bit integers */
XDRLIBAPI bool_t    xdr_int8_t (XDR *xdrs, int8_t *ip);

/* XDR 8bit unsigned integers */
XDRLIBAPI bool_t    xdr_uint8_t (XDR *xdrs, uint8_t *uip);

#ifdef __cplusplus
}
#endif

/*
 * Common opaque bytes objects used by many rpc protocols;
 * declared here due to commonality.
 */
#define MAX_NETOBJ_SZ 1024
struct netobj {
    u_int    n_len;
    char    *n_bytes;
};
typedef struct netobj netobj;
XDRLIBAPI bool_t   xdr_netobj(XDR *,struct netobj *);

/*
 * These are the public routines for the various implementations of
 * xdr streams.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* XDR using memory buffers */
XDRLIBAPI void   xdrmem_create(XDR *, char *, u_int, enum xdr_op);

/* XDR using stdio library */
XDRLIBAPI void   xdrstdio_create(XDR *, FILE *, enum xdr_op);

/* XDR pseudo records for tcp */
XDRLIBAPI void   xdrrec_create(XDR *, u_int, u_int, void *,
                int (*)(void *, void *, int),
                int (*)(void *, void *, int));

/* make end of xdr record */
XDRLIBAPI bool_t xdrrec_endofrecord(XDR *, int);

/* move to beginning of next record */
XDRLIBAPI bool_t xdrrec_skiprecord(XDR *);

/* true if no more input */
XDRLIBAPI bool_t xdrrec_eof(XDR *);

#ifdef __cplusplus
}
#endif

#ifdef HDF
#include "hdf.h"
#endif /* HDF */

#endif /* __XDR_HEADER__ */
