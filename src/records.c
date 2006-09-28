/****************************************************************************
**
*W  records.c                   GAP source                   Martin Schoenert
**
*H  @(#)$Id$
**
*Y  Copyright (C)  1996,  Lehrstuhl D fuer Mathematik,  RWTH Aachen,  Germany
*Y  (C) 1998 School Math and Comp. Sci., University of St.  Andrews, Scotland
*Y  Copyright (C) 2002 The GAP Group
**
**  This file contains the functions of the generic record package.
**
**  This package  provides a uniform  interface to  the functions that access
**  records and the elements for the other packages in the GAP kernel.
*/
#include        "system.h"              /* system dependent part           */

const char * Revision_records_c =
   "@(#)$Id$";

#include        "gasman.h"              /* garbage collector               */
#include        "objects.h"             /* objects                         */
#include        "scanner.h"             /* scanner                         */

#include        "gap.h"                 /* error handling, initialisation  */

#include        "gvars.h"               /* global variables                */
#include        "calls.h"               /* generic call mechanism          */
#include        "opers.h"               /* generic operations              */

#define INCLUDE_DECLARATION_PART
#include        "records.h"             /* generic records                 */
#undef  INCLUDE_DECLARATION_PART

#include        "bool.h"                /* booleans                        */

#include        "records.h"             /* generic records                 */
#include        "precord.h"             /* plain records                   */

#include        "lists.h"               /* generic lists                   */
#include        "plist.h"               /* plain lists                     */
#include        "string.h"              /* strings                         */


/****************************************************************************
**

*F  CountRnam . . . . . . . . . . . . . . . . . . . .  number of record names
**
**  'CountRnam' is the number of record names.
*/
UInt            CountRNam;


/****************************************************************************
**
*F  NAME_RNAM(<rnam>) . . . . . . . . . . . . . . . .  name for a record name
**
**  'NAME_RNAM' returns the name (as a C string) for the record name <rnam>.
**
**  Note that 'NAME_RNAM' is a  macro, so do not call  it with arguments that
**  have sideeffects.
**
**  'NAME_RNAM' is defined in the declaration part of this package as follows
**
#define NAME_RNAM(rnam) CSTR_STRING( ELM_PLIST( NamesRNam, rnam ) )
*/
Obj             NamesRNam;


/****************************************************************************
**
*F  RNamName(<name>)  . . . . . . . . . . . . convert a name to a record name
**
**  'RNamName' returns  the record name with the  name  <name> (which is  a C
**  string).
*/
Obj             HashRNam;

UInt            SizeRNam;

UInt            RNamName (
    Char *              name )
{
    Obj                 rnam;           /* record name (as imm intobj)     */
    UInt                pos;            /* hash position                   */
    Char                namx [1024];    /* temporary copy of <name>        */
    Obj                 string;         /* temporary string object <name>  */
    Obj                 table;          /* temporary copy of <HashRNam>    */
    Obj                 rnam2;          /* one element of <table>          */
    Char *              p;              /* loop variable                   */
    UInt                i;              /* loop variable                   */

    /* start looking in the table at the following hash position           */
    pos = 0;
    for ( p = name; *p != '\0'; p++ ) {
        pos = 65599 * pos + *p;
    }
    pos = (pos % SizeRNam) + 1;

    /* look through the table until we find a free slot or the global      */
    while ( (rnam = ELM_PLIST( HashRNam, pos )) != 0
         && SyStrncmp( NAME_RNAM( INT_INTOBJ(rnam) ), name, 1023 ) ) {
        pos = (pos % SizeRNam) + 1;
    }

    /* if we did not find the global variable, make a new one and enter it */
    /* (copy the name first, to avoid a stale pointer in case of a GC)     */
    if ( rnam == 0 ) {
        CountRNam++;
        rnam = INTOBJ_INT(CountRNam);
        SET_ELM_PLIST( HashRNam, pos, rnam );
        namx[0] = '\0';
        SyStrncat( namx, name, 1023 );
        string = NEW_STRING( SyStrlen(namx) );
        SyStrncat( CSTR_STRING(string), namx, SyStrlen(namx) );
        GROW_PLIST(    NamesRNam,   CountRNam );
        SET_LEN_PLIST( NamesRNam,   CountRNam );
        SET_ELM_PLIST( NamesRNam,   CountRNam, string );
        CHANGED_BAG(   NamesRNam );
    }

    /* if the table is too crowed, make a larger one, rehash the names     */
    if ( SizeRNam < 3 * CountRNam / 2 ) {
        table = HashRNam;
        SizeRNam = 2 * SizeRNam + 1;
        HashRNam = NEW_PLIST( T_PLIST, SizeRNam );
        SET_LEN_PLIST( HashRNam, SizeRNam );
        for ( i = 1; i <= (SizeRNam-1)/2; i++ ) {
            rnam2 = ELM_PLIST( table, i );
            if ( rnam2 == 0 )  continue;
            pos = 0;
            for ( p = NAME_RNAM( INT_INTOBJ(rnam2) ); *p != '\0'; p++ ) {
                pos = 65599 * pos + *p;
            }
            pos = (pos % SizeRNam) + 1;
            while ( ELM_PLIST( HashRNam, pos ) != 0 ) {
                pos = (pos % SizeRNam) + 1;
            }
            SET_ELM_PLIST( HashRNam, pos, rnam2 );
        }
    }

    /* return the record name                                              */
    return INT_INTOBJ(rnam);
}


/****************************************************************************
**
*F  RNamIntg(<intg>)  . . . . . . . . . . convert an integer to a record name
**
**  'RNamIntg' returns the record name corresponding to the integer <intg>.
*/
UInt            RNamIntg (
    Int                 intg )
{
    Char                name [16];      /* integer converted to a string   */
    Char *              p;              /* loop variable                   */

    /* convert the integer to a string                                     */
    p = name + sizeof(name);  *--p = '\0';
    do {
        *--p = '0' + intg % 10;
    } while ( (intg /= 10) != 0 );

    /* return the name                                                     */
    return RNamName( p );
}


/****************************************************************************
**
*F  RNamObj(<obj>)  . . . . . . . . . . .  convert an object to a record name
**
**  'RNamObj' returns the record name  corresponding  to  the  object  <obj>,
**  which currently must be a string or an integer.
*/
UInt            RNamObj (
    Obj                 obj )
{
    /* convert integer object                                              */
    if ( IS_INTOBJ(obj) ) {
        return RNamIntg( INT_INTOBJ(obj) );
    }

    /* convert string object (empty string may have type T_PLIST)          */
    else if ( IsStringConv(obj) && MUTABLE_TNUM(TNUM_OBJ(obj))==T_STRING ) {
        return RNamName( CSTR_STRING(obj) );
    }

    /* otherwise fail                                                      */
    else {
        obj = ErrorReturnObj(
            "Record: '<rec>.(<obj>)' <obj> must be a string or an integer",
            0L, 0L,
            "you can replace <obj> via 'return <obj>;'" );
        return RNamObj( obj );
    }
}


/****************************************************************************
**
*F  RNamObjHandler(<self>,<obj>)  . . . .  convert an object to a record name
**
**  'RNamObjHandler' implements the internal function 'RNamObj'.
**
**  'RNamObj( <obj> )'
**
**  'RNamObj' returns the record name  corresponding  to  the  object  <obj>,
**  which currently must be a string or an integer.
*/
Obj             RNamObjHandler (
    Obj                 self,
    Obj                 obj )
{
    return INTOBJ_INT( RNamObj( obj ) );
}


/****************************************************************************
**
*F  NameRNamHandler(<self>,<rnam>)  . . . . convert a record name to a string
**
**  'NameRNamHandler' implements the internal function 'NameRName'.
**
**  'NameRName( <rnam> )'
**
**  'NameRName' returns the string corresponding to the record name <rnam>.
*/
Obj             NameRNamFunc;

Obj             NameRNamHandler (
    Obj                 self,
    Obj                 rnam )
{
    Obj                 name;
    while ( ! IS_INTOBJ(rnam)
         || INT_INTOBJ(rnam) <= 0
        || CountRNam < INT_INTOBJ(rnam) ) {
        rnam = ErrorReturnObj(
            "NameRName: <rnam> must be a record name (not a %s)",
            (Int)TNAM_OBJ(rnam), 0L,
            "you can replace <rnam> via 'return <rnam>;'" );
    }
    name = NEW_STRING( SyStrlen( NAME_RNAM( INT_INTOBJ(rnam) ) ) );
    SyStrncat( CSTR_STRING(name),
               NAME_RNAM( INT_INTOBJ(rnam) ),
               SyStrlen( NAME_RNAM( INT_INTOBJ(rnam) ) ) );
    return name;
}


/****************************************************************************
**
*F  IS_REC(<obj>) . . . . . . . . . . . . . . . . . . . is an object a record
*V  IsRecFuncs[<type>]  . . . . . . . . . . . . . . . . table of record tests
**
**  'IS_REC' returns a nonzero value if the object <obj> is a  record  and  0
**  otherwise.
**
**  Note that 'IS_REC' is a macro, so do not call  it  with  arguments  that
**  have sideeffects.
**
**  'IS_REC' is defined in the declaration part of this package as follows
**
#define IS_REC(obj)     ((*IsRecFuncs[ TNUM_OBJ(obj) ])( obj ))
*/
Int             (*IsRecFuncs[LAST_REAL_TNUM+1]) ( Obj obj );

Obj             IsRecFilt;

Obj             IsRecHandler (
    Obj                 self,
    Obj                 obj )
{
    return (IS_REC(obj) ? True : False);
}

Int             IsRecNot (
    Obj                 obj )
{
    return 0L;
}

Int             IsRecYes (
    Obj                 obj )
{
    return 1L;
}

Int             IsRecObject (
    Obj                 obj )
{
    return (DoFilter( IsRecFilt, obj ) == True);
}


/****************************************************************************
**
*F  ELM_REC(<rec>,<rnam>) . . . . . . . . . . select an element from a record
**
**  'ELM_REC' returns the element, i.e., the value of the component, with the
**  record name <rnam> in  the record <rec>.   An error is signalled if <rec>
**  is not a record or if <rec> has no component with the record name <rnam>.
**
**  Note that 'ELM_REC' is  a macro, so do   not call it with arguments  that
**  have sideeffects.
**
**  'ELM_REC' is defined in the declaration part of this package as follows
**
#define ELM_REC(rec,rnam) \
                        ((*ElmRecFuncs[ TNUM_OBJ(rec) ])( rec, rnam ))
*/
Obj             (*ElmRecFuncs[LAST_REAL_TNUM+1]) ( Obj rec, UInt rnam );

Obj             ElmRecOper;

Obj             ElmRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam )
{
    return ELM_REC( rec, INT_INTOBJ(rnam) );
}

Obj             ElmRecError (
    Obj                 rec,
    UInt                rnam )
{
    rec = ErrorReturnObj(
        "Record Element: <rec> must be a record (not a %s)",
        (Int)TNAM_OBJ(rec), 0L,
        "you can replace <rec> via 'return <rec>;'" );
    return ELM_REC( rec, rnam );
}

Obj             ElmRecObject (
    Obj                 obj,
    UInt                rnam )
{
  Obj elm;
  elm = DoOperation2Args( ElmRecOper, obj, INTOBJ_INT(rnam) );
  while (elm == 0)
    elm =  ErrorReturnObj("Record access method must return a value",0L,0L,
                          "you can supply a value <val> via 'return <val>;'");
  return elm;

}


/****************************************************************************
**
*F  ISB_REC(<rec>,<rnam>) . . . . . . . . . test for an element from a record
**
**  'ISB_REC' returns 1 if the record <rec> has a component with  the  record
**  name <rnam> and 0 otherwise.  An error is signalled if  <rec>  is  not  a
**  record.
**
**  Note  that 'ISB_REC'  is a macro,  so do not call  it with arguments that
**  have sideeffects.
**
**  'ISB_REC' is defined in the declaration part of this package as follows
**
#define ISB_REC(rec,rnam) \
                        ((*IsbRecFuncs[ TNUM_OBJ(rec) ])( rec, rnam ))
*/
Int             (*IsbRecFuncs[LAST_REAL_TNUM+1]) ( Obj rec, UInt rnam );

Obj             IsbRecOper;

Obj             IsbRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam )
{
    return (ISB_REC( rec, INT_INTOBJ(rnam) ) ? True : False);
}

Int             IsbRecError (
    Obj                 rec,
    UInt                rnam )
{
    rec = ErrorReturnObj(
        "IsBound: <rec> must be a record (not a %s)",
        (Int)TNAM_OBJ(rec), 0L,
        "you can replace <rec> via 'return <rec>;'" );
    return ISB_REC( rec, rnam );
}

Int             IsbRecObject (
    Obj                 obj,
    UInt                rnam )
{
    return (DoOperation2Args( IsbRecOper, obj, INTOBJ_INT(rnam) ) == True);
}


/****************************************************************************
**
*F  ASS_REC(<rec>,<rnam>,<obj>) . . . . . . . . . . . . .  assign to a record
**
**  'ASS_REC' assigns the object <obj>  to  the  record  component  with  the
**  record name <rnam> in the record <rec>.  An error is signalled  if  <rec>
**  is not a record.
**
**  'ASS_REC' is defined in the declaration part of this package as follows
**
#define ASS_REC(rec,rnam,obj) \
                        ((*AssRecFuncs[ TNUM_OBJ(rec) ])( rec, rnam, obj ))
*/
void            (*AssRecFuncs[LAST_REAL_TNUM+1]) ( Obj rec, UInt rnam, Obj obj );

Obj             AssRecOper;

Obj             AssRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam,
    Obj                 obj )
{
    ASS_REC( rec, INT_INTOBJ(rnam), obj );
    return 0;
}

void            AssRecError (
    Obj                 rec,
    UInt                rnam,
    Obj                 obj )
{
    rec = ErrorReturnObj(
        "Record Assignment: <rec> must be a record (not a %s)",
        (Int)TNAM_OBJ(rec), 0L,
        "you can replace <rec> via 'return <rec>;'" );
    ASS_REC( rec, rnam, obj );
}

void            AssRecObject (
    Obj                 obj,
    UInt                rnam,
    Obj                 val )
{
    DoOperation3Args( AssRecOper, obj, INTOBJ_INT(rnam), val );
}


/****************************************************************************
**
*F  UNB_REC(<rec>,<rnam>) . . . . . . unbind a record component from a record
**
**  'UNB_REC' removes the record component  with the record name <rnam>  from
**  the record <rec>.
**
**  Note that 'UNB_REC' is  a macro, so  do  not call it with  arguments that
**  have sideeffects.
**
**  'UNB_REC' is defined in the declaration part of this package as follows
**
#define UNB_REC(rec,rnam) \
                        ((*UnbRecFuncs[ TNUM_OBJ(rec) ])( rec, rnam ))
*/
void            (*UnbRecFuncs[LAST_REAL_TNUM+1]) ( Obj rec, UInt rnam );

Obj             UnbRecOper;

Obj             UnbRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam )
{
    UNB_REC( rec, INT_INTOBJ(rnam) );
    return 0;
}

void            UnbRecError (
    Obj                 rec,
    UInt                rnam )
{
    rec = ErrorReturnObj(
        "Unbind: <rec> must be a record (not a %s)",
        (Int)TNAM_OBJ(rec), 0L,
        "you can replace <rec> via 'return <rec>;'" );
    UNB_REC( rec, rnam );
}
        
void            UnbRecObject (
    Obj                 obj,
    UInt                rnam )
{
    DoOperation2Args( UnbRecOper, obj, INTOBJ_INT(rnam) );
}


/****************************************************************************
**
*F  iscomplete( <name>, <len> ) . . . . . . . .  find the completions of name
*F  completion( <name>, <len> ) . . . . . . . .  find the completions of name
*/
UInt            iscomplete_rnam (
    Char *              name,
    UInt                len )
{
    Char *              curr;
    UInt                i, k;

    for ( i = 1; i <= CountRNam; i++ ) {
        curr = NAME_RNAM( i );
        for ( k = 0; name[k] != 0 && curr[k] == name[k]; k++ ) ;
        if ( k == len && curr[k] == '\0' )  return 1;
    }
    return 0;
}

UInt            completion_rnam (
    Char *              name,
    UInt                len )
{
    Char *              curr;
    Char *              next;
    UInt                i, k;

    next = 0;
    for ( i = 1; i <= CountRNam; i++ ) {
        curr = NAME_RNAM( i );
        for ( k = 0; name[k] != 0 && curr[k] == name[k]; k++ ) ;
        if ( k < len || curr[k] <= name[k] )  continue;
        if ( next != 0 ) {
            for ( k = 0; curr[k] != '\0' && curr[k] == next[k]; k++ ) ;
            if ( k < len || next[k] < curr[k] )  continue;
        }
        next = curr;
    }

    if ( next != 0 ) {
        for ( k = 0; next[k] != '\0'; k++ )
            name[k] = next[k];
        name[k] = '\0';
    }

    return next != 0;
}

Obj FuncALL_RNAMES (
    Obj                 self )
{
    Obj                 copy, s;
    UInt                i;
    Char*               name;

    copy = NEW_PLIST( T_PLIST+IMMUTABLE, CountRNam );
    for ( i = 1;  i <= CountRNam;  i++ ) {
        name = NAME_RNAM( i );
        C_NEW_STRING(s, strlen(name), name);
        SET_ELM_PLIST( copy, i, s );
    }
    SET_LEN_PLIST( copy, CountRNam );
    return copy;
}

/****************************************************************************
**

*F * * * * * * * * * * * * * initialize package * * * * * * * * * * * * * * *
*/

/****************************************************************************
**

*V  GVarFilts . . . . . . . . . . . . . . . . . . . list of filters to export
*/
static StructGVarFilt GVarFilts [] = {

    { "IS_REC", "obj", &IsRecFilt,
      IsRecHandler, "src/records.c:IS_REC" },

    { 0 }

};


/****************************************************************************
**
*V  GVarOpers . . . . . . . . . . . . . . . . .  list of operations to export
*/
static StructGVarOper GVarOpers [] = {

    { "ELM_REC",  2, "obj, rnam", &ElmRecOper, 
      ElmRecHandler, "src/records.c:ELM_REC" },

    { "ISB_REC",  2, "obj, rnam", &IsbRecOper, 
      IsbRecHandler, "src/records.c:ISB_REC" },

    { "ASS_REC",  3, "obj, rnam, val", &AssRecOper, 
      AssRecHandler, "src/records.c:ASS_REC" },

    { "UNB_REC",  2, "obj, rnam", &UnbRecOper, 
      UnbRecHandler, "src/records.c:UNB_REC" },

    { 0 }

};


/****************************************************************************
**
*V  GVarFuncs . . . . . . . . . . . . . . . . . . list of functions to export
*/
static StructGVarFunc GVarFuncs [] = {

    { "RNamObj", 1, "obj",
      RNamObjHandler, "src/records.c:RNamObj" },

    { "NameRNam", 1, "rnam",
      NameRNamHandler, "src/records.c:NameRNam" },

    { "ALL_RNAMES", 0, "",
      FuncALL_RNAMES, "src/records.c:ALL_RNAMES" },

    { 0 }

};


/****************************************************************************
**

*F  InitKernel( <module> )  . . . . . . . . initialise kernel data structures
*/
static Int InitKernel (
    StructInitInfo *    module )
{
    UInt                type;           /* loop variable                   */

    /* make the list of names of record names                              */
    InitGlobalBag( &NamesRNam, "src/records.c:NamesRNam" );

    /* make the hash list of record names                                  */
    InitGlobalBag( &HashRNam, "src/records.c:HashRNam" );

    /* init filters and functions                                          */
    InitHdlrFiltsFromTable( GVarFilts );
    InitHdlrOpersFromTable( GVarOpers );
    InitHdlrFuncsFromTable( GVarFuncs );

    /* make and install the 'IS_REC' filter                                */
    for ( type = FIRST_REAL_TNUM; type <= LAST_REAL_TNUM; type++ ) {
        IsRecFuncs[ type ] = IsRecNot;
    }
    for ( type = FIRST_RECORD_TNUM; type <= LAST_RECORD_TNUM; type++ ) {
        IsRecFuncs[ type ] = IsRecYes;
    }
    for ( type = FIRST_EXTERNAL_TNUM; type <= LAST_EXTERNAL_TNUM; type++ ) {
        IsRecFuncs[ type ] = IsRecObject;
    }


    /* make and install the 'ELM_REC' operations                           */
    for ( type = FIRST_REAL_TNUM; type <= LAST_REAL_TNUM; type++ ) {
        ElmRecFuncs[ type ] = ElmRecError;
    }
    for ( type = FIRST_EXTERNAL_TNUM; type <= LAST_EXTERNAL_TNUM; type++ ) {
        ElmRecFuncs[ type ] = ElmRecObject;
    }


    /* make and install the 'ISB_REC' operation                            */
    for ( type = FIRST_REAL_TNUM; type <= LAST_REAL_TNUM; type++ ) {
        IsbRecFuncs[ type ] = IsbRecError;
    }
    for ( type = FIRST_EXTERNAL_TNUM; type <= LAST_EXTERNAL_TNUM; type++ ) {
        IsbRecFuncs[ type ] = IsbRecObject;
    }


    /* make and install the 'ASS_REC' operation                            */
    for ( type = FIRST_REAL_TNUM; type <= LAST_REAL_TNUM; type++ ) {
        AssRecFuncs[ type ] = AssRecError;
    }
    for ( type = FIRST_EXTERNAL_TNUM; type <= LAST_EXTERNAL_TNUM; type++ ) {
        AssRecFuncs[ type ] = AssRecObject;
    }


    /* make and install the 'UNB_REC' operation                            */
    for ( type = FIRST_REAL_TNUM; type <= LAST_REAL_TNUM; type++ ) {
        UnbRecFuncs[ type ] = UnbRecError;
    }
    for ( type = FIRST_EXTERNAL_TNUM; type <= LAST_EXTERNAL_TNUM; type++ ) {
        UnbRecFuncs[ type ] = UnbRecObject;
    }

    /* return success                                                      */
    return 0;
}


/****************************************************************************
**
*F  PostRestore( <module> ) . . . . . . . . . . . . . after restore workspace
*/
static Int PostRestore (
    StructInitInfo *    module )
{
    /* make the list of names of record names                              */
    CountRNam = LEN_PLIST(NamesRNam);

    /* make the hash list of record names                                  */
    SizeRNam = LEN_PLIST(HashRNam);

    /* return success                                                      */
    return 0;
}


/****************************************************************************
**
*F  InitLibrary( <module> ) . . . . . . .  initialise library data structures
*/
static Int InitLibrary (
    StructInitInfo *    module )
{
    /* make the list of names of record names                              */
    CountRNam = 0;
    NamesRNam = NEW_PLIST( T_PLIST, 0 );
    SET_LEN_PLIST( NamesRNam, 0 );

    /* make the hash list of record names                                  */
    SizeRNam = 997;
    HashRNam = NEW_PLIST( T_PLIST, SizeRNam );
    SET_LEN_PLIST( HashRNam, SizeRNam );

    /* init filters and functions                                          */
    InitGVarFiltsFromTable( GVarFilts );
    InitGVarOpersFromTable( GVarOpers );
    InitGVarFuncsFromTable( GVarFuncs );

    /* return success                                                      */
    return 0;
}


/****************************************************************************
**
*F  InitInfoRecords() . . . . . . . . . . . . . . . . table of init functions
*/
static StructInitInfo module = {
    MODULE_BUILTIN,                     /* type                           */
    "records",                          /* name                           */
    0,                                  /* revision entry of c file       */
    0,                                  /* revision entry of h file       */
    0,                                  /* version                        */
    0,                                  /* crc                            */
    InitKernel,                         /* initKernel                     */
    InitLibrary,                        /* initLibrary                    */
    0,                                  /* checkInit                      */
    0,                                  /* preSave                        */
    0,                                  /* postSave                       */
    PostRestore                         /* postRestore                    */
};

StructInitInfo * InitInfoRecords ( void )
{
    module.revision_c = Revision_records_c;
    module.revision_h = Revision_records_h;
    FillInVersion( &module );
    return &module;
}


/****************************************************************************
**

*E  records.c . . . . . . . . . . . . . . . . . . . . . . . . . . . ends here
*/

