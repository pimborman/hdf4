/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

#ifdef RCSID
static char RcsId[] = "@(#)$Revision$";
#endif
/* $Id$ */

/*****************************************************************************
*
* vparse.c
* Part of the HDF VSet interface.
*
************************************************************************/

#include "vg.h"

#define ISCOMMA(c) ( (c==',') ? 1:0 )

/* ------------------------------------------------------------------ */

/*
   ** Given a string (attrs) , the routine parses it into token strings,
   ** and returns a ptr (attrv) to an array of ptrs where the tokens
   ** are stored.  The number of tokens are returned in attrc.
   **
   ** Currently used only by routines that manipulate field names.
   ** As such each field string is truncated to a max length of
   ** FIELDNAMELENMAX (as defined in hdf.h). For most cases, this
   ** truncation doesn't happen because FIELDNAMELENMAX is a big number.
   **
   ** RETURN FAIL if error.
   ** RETURN SUCCEED if ok.
   **
   ** Current implementation: all strings inputs converted to uppercase.
   ** tokens must be separated by COMMAs.
   **
   ** Tokens are stored in static area sym , and pointers are returned
   ** to calling routine. Hence, tokens must be used before next call
   ** to scanattrs.
   **
 */
#if defined(macintosh) || defined(MAC) || defined(__MWERKS__) || defined(SYMANTEC_C) || defined(DMEM)   /* Dynamic memory */
PRIVATE char **symptr = NULL;   /* array of ptrs to tokens  ? */
PRIVATE char **sym = NULL;      /* array of tokens ? */
#else  /* !macintosh */
PRIVATE char *symptr[VSFIELDMAX];       /* array of ptrs to tokens  ? */
PRIVATE char sym[VSFIELDMAX][FIELDNAMELENMAX + 1];  /* array of tokens ? */
#endif /* !macintosh */
PRIVATE intn nsym;              /* token index ? */

/* Temporary buffer for I/O */
PRIVATE uint32 Vpbufsize = 0;
PRIVATE uint8 *Vpbuf = NULL;

int32 scanattrs(const char *attrs, int32 *attrc, char ***attrv)
{
    CONSTR(FUNC, "scanattrs");
    char *s, *s0, *ss;
    intn len;
    size_t slen = HDstrlen(attrs)+1;

#if defined(macintosh) || defined(MAC) || defined(__MWERKS__) || defined(SYMANTEC_C) || defined(DMEM)   /* Dynamic memory */
    intn i;

    /* Lets allocate space for ptrs to tokens and tokens */
    if (symptr == NULL)
      {
          symptr = (char **) HDmalloc(VSFIELDMAX * sizeof(char *));
          if (symptr == NULL)
              HRETURN_ERROR(DFE_NOSPACE, FAIL);
      }

    if (sym == NULL)
      {
          sym = (char **) HDmalloc(VSFIELDMAX * sizeof(char *));
          if (sym == NULL)
              HRETURN_ERROR(DFE_NOSPACE, FAIL);

          for (i = 0; i < VSFIELDMAX; i++)
            {
                sym[i] = (char *) HDmalloc(sizeof(char) * (FIELDNAMELENMAX + 1));
                if (sym[i] == NULL)
                    HRETURN_ERROR(DFE_NOSPACE, FAIL);
            }
      }

#endif /* macintosh */

    if(slen>Vpbufsize)
      {
        Vpbufsize = slen;
        if (Vpbuf)
            HDfree((VOIDP) Vpbuf);
        if ((Vpbuf = (uint8 *) HDmalloc(Vpbufsize)) == NULL)
            HRETURN_ERROR(DFE_NOSPACE, FAIL);
      } /* end if */

    HDstrcpy((char *)Vpbuf,attrs);
    s = (char *)Vpbuf;
    nsym = 0;

    s0 = s;
    while (*s)
      {

#ifdef VDATA_FIELDS_ALL_UPPER
          if (*s >= 'a' && *s <= 'z')
              *s = (char) toupper(*s);
#endif /* VDATA_FIELDS_ALL_UPPER */

          if (ISCOMMA(*s))
            {

                /* make sure we've got a legitimate length */
                len = (intn) (s - s0);
                if (len <= 0)
                    return (FAIL);

                /* save that token */
                ss = symptr[nsym] = sym[nsym];
                nsym++;

                /* shove the string into our static buffer.  YUCK! */
                if (len > FIELDNAMELENMAX)
                    len = FIELDNAMELENMAX;
                HIstrncpy(ss, s0, len + 1);

                /* skip over the comma */
                s++;

                /* skip over white space before the next field name */
                while (*s && *s == ' ')
                    s++;

                /* keep track of the first character of the next token */
                s0 = s;

            }
          else
            {

                /* move along --- nothing to see here */
                s++;
            }
      }

    /* save the last token */
    len = (intn) (s - s0);
    if (len <= 0)
        return (FAIL);
    ss = symptr[nsym] = sym[nsym];
    nsym++;

    if (len > FIELDNAMELENMAX)
        len = FIELDNAMELENMAX;
    HIstrncpy(ss, s0, len + 1);

    symptr[nsym] = NULL;
    *attrc = nsym;
    *attrv = (char **) symptr;

    return (SUCCEED);   /* ok */
}   /* scanattrs */

/*--------------------------------------------------------------------------
 NAME
    VPparse_shutdown
 PURPOSE
    Free the Vpbuf buffer.
 USAGE
    intn VPparse_shutdown()
 RETURNS
    Returns SUCCEED/FAIL
 DESCRIPTION
    For completeness, when the VSet interface is shut-down, free the Vpbuf.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Should only ever be called by the "atexit" function HDFend
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void VPparse_shutdown(void)
{
    if(Vpbuf!=NULL)
      {
        HDfree(Vpbuf);
        Vpbuf=NULL;
        Vpbufsize = 0;
      } /* end if */
} /* end VSPhshutdown() */

/* ------------------------------------------------------------------ */
