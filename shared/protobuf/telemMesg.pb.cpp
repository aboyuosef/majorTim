/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9 at Sun Mar 25 13:53:52 2018. */

#include "telemMesg.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t TelemMsg_fields[4] = {
    PB_FIELD(  1, INT64   , SINGULAR, STATIC  , FIRST, TelemMsg, timestamp, timestamp, 0),
    PB_FIELD(  2, DOUBLE  , REPEATED, STATIC  , OTHER, TelemMsg, data, timestamp, 0),
    PB_FIELD(  3, FIXED32 , SINGULAR, STATIC  , OTHER, TelemMsg, validDataIndexes, data, 0),
    PB_LAST_FIELD
};


/* On some platforms (such as AVR), double is really float.
 * These are not directly supported by nanopb, but see example_avr_double.
 * To get rid of this error, remove any double fields from your .proto.
 */
PB_STATIC_ASSERT(sizeof(double) == 8, DOUBLE_MUST_BE_8_BYTES)

/* @@protoc_insertion_point(eof) */
