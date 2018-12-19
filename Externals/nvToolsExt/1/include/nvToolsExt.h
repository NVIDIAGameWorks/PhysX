/*
* Copyright 2009-2012  NVIDIA Corporation.  All rights reserved.
*
* NOTICE TO USER:
*
* This source code is subject to NVIDIA ownership rights under U.S. and
* international Copyright laws.
*
* This software and the information contained herein is PROPRIETARY and
* CONFIDENTIAL to NVIDIA and is being provided under the terms and conditions
* of a form of NVIDIA software license agreement.
*
* NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
* CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
* IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
* REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
* OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
* OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
* OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
* OR PERFORMANCE OF THIS SOURCE CODE.
*
* U.S. Government End Users.   This source code is a "commercial item" as
* that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
* "commercial computer  software"  and "commercial computer software
* documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
* and is provided to the U.S. Government only as a commercial end item.
* Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
* 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
* source code with only those rights set forth herein.
*
* Any use of this source code in individual and commercial software must
* include, in the user documentation and internal comments to the code,
* the above Disclaimer and U.S. Government End Users Notice.
*/

/** \mainpage
 * \section Introduction
 * The NVIDIA Tools Extension library is a set of functions that a
 * developer can use to provide additional information to tools.
 * The additional information is used by the tool to improve
 * analysis and visualization of data.
 *
 * The library introduces close to zero overhead if no tool is
 * attached to the application.  The overhead when a tool is
 * attached is specific to the tool.
 */

#ifndef NVTOOLSEXT_H_
#define NVTOOLSEXT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(_MSC_VER) /* Microsoft Visual C++ Compiler */
    #ifdef NVTX_EXPORTS
        #define NVTX_DECLSPEC
    #else
        #define NVTX_DECLSPEC __declspec(dllimport)
    #endif /* NVTX_EXPORTS */
    #define NVTX_API __stdcall
#else /* GCC and most other compilers */
    #define NVTX_DECLSPEC
    #define NVTX_API
#endif /* Platform */

/**
 * The nvToolsExt library depends on stdint.h.  If the build tool chain in use
 * does not include stdint.h then define NVTX_STDINT_TYPES_ALREADY_DEFINED
 * and define the following types:
 * <ul>
 *   <li>uint8_t
 *   <li>int8_t
 *   <li>uint16_t
 *   <li>int16_t
 *   <li>uint32_t
 *   <li>int32_t
 *   <li>uint64_t
 *   <li>int64_t
 *   <li>uintptr_t
 *   <li>intptr_t
 * </ul>
 #define NVTX_STDINT_TYPES_ALREADY_DEFINED if you are using your own header file.
 */
#ifndef NVTX_STDINT_TYPES_ALREADY_DEFINED
#include <stdint.h>
#endif

/**
 * Tools Extension API version
 */
#define NVTX_VERSION 1

/**
 * Size of the nvtxEventAttributes_t structure.
 */
#define NVTX_EVENT_ATTRIB_STRUCT_SIZE ( (uint16_t)( sizeof(nvtxEventAttributes_v1) ) )

typedef uint64_t nvtxRangeId_t;

/** \page EVENT_ATTRIBUTES Event Attributes
 *
 * \ref MARKER_AND_RANGES can be annotated with various attributes to provide
 * additional information for an event or to guide the tool's visualization of
 * the data. Each of the attributes is optional and if left unused the
 * attributes fall back to a default value.
 *
 * To specify any attribute other than the text message, the \ref
 * EVENT_ATTRIBUTE_STRUCTURE "Event Attribute Structure" must be used.
 */

/** ---------------------------------------------------------------------------
 * Color Types
 * ------------------------------------------------------------------------- */
typedef enum nvtxColorType_t
{
    NVTX_COLOR_UNKNOWN  = 0,                 /**< Color attribute is unused. */
    NVTX_COLOR_ARGB     = 1                  /**< An ARGB color is provided. */
} nvtxColorType_t;

/** ---------------------------------------------------------------------------
 * Payload Types
 * ------------------------------------------------------------------------- */
typedef enum nvtxPayloadType_t
{
    NVTX_PAYLOAD_UNKNOWN                = 0,   /**< Color payload is unused. */
    NVTX_PAYLOAD_TYPE_UNSIGNED_INT64    = 1,   /**< A unsigned integer value is used as payload. */
    NVTX_PAYLOAD_TYPE_INT64             = 2,   /**< A signed integer value is used as payload. */
    NVTX_PAYLOAD_TYPE_DOUBLE            = 3    /**< A floating point value is used as payload. */
} nvtxPayloadType_t;

/** ---------------------------------------------------------------------------
 * Message Types
 * ------------------------------------------------------------------------- */
typedef enum nvtxMessageType_t
{
    NVTX_MESSAGE_UNKNOWN        = 0,         /**< Message payload is unused. */
    NVTX_MESSAGE_TYPE_ASCII     = 1,         /**< A character sequence is used as payload. */
    NVTX_MESSAGE_TYPE_UNICODE   = 2          /**< A wide character sequence is used as payload. */
} nvtxMessageType_t;

/** \brief Event Attribute Structure.
 * \anchor EVENT_ATTRIBUTE_STRUCTURE
 *
 * This structure is used to describe the attributes of an event. The layout of
 * the structure is defined by a specific version of the tools extension
 * library and can change between different versions of the Tools Extension
 * library.
 *
 * \par Initializing the Attributes
 *
 * The caller should always perform the following three tasks when using
 * attributes:
 * <ul>
 *    <li>Zero the structure
 *    <li>Set the version field
 *    <li>Set the size field
 * </ul>
 *
 * Zeroing the structure sets all the event attributes types and values
 * to the default value.
 *
 * The version and size field are used by the Tools Extension
 * implementation to handle multiple versions of the attributes structure.
 *
 * It is recommended that the caller use one of the following to methods
 * to initialize the event attributes structure:
 *
 * \par Method 1: Initializing nvtxEventAttributes for future compatibility
 * \code
 * nvtxEventAttributes_t eventAttrib = {0};
 * eventAttrib.version = NVTX_VERSION;
 * eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
 * \endcode
 *
 * \par Method 2: Initializing nvtxEventAttributes for a specific version
 * \code
 * nvtxEventAttributes_t eventAttrib = {0};
 * eventAttrib.version = 1;
 * eventAttrib.size = (uint16_t)(sizeof(nvtxEventAttributes_v1));
 * \endcode
 *
 * If the caller uses Method 1 it is critical that the entire binary
 * layout of the structure be configured to 0 so that all fields
 * are initialized to the default value.
 *
 * The caller should either use both NVTX_VERSION and
 * NVTX_EVENT_ATTRIB_STRUCT_SIZE (Method 1) or use explicit values
 * and a versioned type (Method 2).  Using a mix of the two methods
 * will likely cause either source level incompatibility or binary
 * incompatibility in the future.
 *
 * \par Settings Attribute Types and Values
 *
 *
 * \par Example:
 * \code
 * // Initialize
 * nvtxEventAttributes_t eventAttrib = {0};
 * eventAttrib.version = NVTX_VERSION;
 * eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
 *
 * // Configure the Attributes
 * eventAttrib.colorType = NVTX_COLOR_ARGB;
 * eventAttrib.color = 0xFF880000;
 * eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
 * eventAttrib.message.ascii = "Example";
 * \endcode
 *
 * In the example the caller does not have to set the value of
 * \ref ::nvtxEventAttributes_v1::category or
 * \ref ::nvtxEventAttributes_v1::payload as these fields were set to
 * the default value by {0}.
 * \sa
 * ::nvtxMarkEx
 * ::nvtxRangeStartEx
 * ::nvtxRangePushEx
 */
typedef struct nvtxEventAttributes_v1
{
    /**
    * \brief Version flag of the structure.
    *
    * Needs to be set to NVTX_VERSION to indicate the version of NVTX APIs
    * supported in this header file. This can optionally be overridden to
    * another version of the tools extension library.
    */
    uint16_t version;

    /**
    * \brief Size of the structure.
    *
    * Needs to be set to the size in bytes of the event attribute
    * structure used to specify the event.
    */
    uint16_t size;

    /**
     * \brief ID of the category the event is assigned to.
     *
     * A category is a user-controlled ID that can be used to group
     * events.  The tool may use category IDs to improve filtering or
     * enable grouping of events in the same category. The functions
     * \ref ::nvtxNameCategoryA or \ref ::nvtxNameCategoryW can be used
     * to name a category.
     *
     * Default Value is 0
     */
    uint32_t category;

    /** \brief Color type specified in this attribute structure.
     *
     * Defines the color format of the attribute structure's \ref COLOR_FIELD
     * "color" field.
     *
     * Default Value is NVTX_COLOR_UNKNOWN
     */
    int32_t colorType;              /* nvtxColorType_t */

    /** \brief Color assigned to this event. \anchor COLOR_FIELD
     *
     * The color that the tool should use to visualize the event.
     */
    uint32_t color;

    /**
     * \brief Payload type specified in this attribute structure.
     *
     * Defines the payload format of the attribute structure's \ref PAYLOAD_FIELD
     * "payload" field.
     *
     * Default Value is NVTX_PAYLOAD_UNKNOWN
     */
    int32_t payloadType;            /* nvtxPayloadType_t */

    /**
     * \brief Payload assigned to this event. \anchor PAYLOAD_FIELD
     *
     * A numerical value that can be used to annotate an event. The tool could
     * use the payload data to reconstruct graphs and diagrams.
     */
    union payload_t
    {
        uint64_t ullValue;
        int64_t llValue;
        double dValue;
    } payload;

    /** \brief Message type specified in this attribute structure.
     *
     * Defines the message format of the attribute structure's \ref MESSAGE_FIELD
     * "message" field.
     *
     * Default Value is NVTX_MESSAGE_UNKNOWN
     */
    int32_t messageType;            /* nvtxMessageType_t */

    /** \brief Message assigned to this attribute structure. \anchor MESSAGE_FIELD
     *
     * The text message that is attached to an event.
     */
    union message_t
    {
        const char* ascii;
        const wchar_t* unicode;
    } message;

} nvtxEventAttributes_v1;

typedef struct nvtxEventAttributes_v1 nvtxEventAttributes_t;

/* ========================================================================= */
/** \defgroup MARKER_AND_RANGES Marker and Ranges
 *
 * Markers and ranges are used to describe events at a specific time (markers)
 * or over a time span (ranges) during the execution of the application
 * respectively. The additional information is presented alongside all other
 * captured data and facilitates understanding of the collected information.
 */

/* ========================================================================= */
/** \name Markers
 */
/** \name Markers
 */
/** \addtogroup MARKER_AND_RANGES
 * \section MARKER Marker
 *
 * A marker describes a single point in time.  A marker event has no side effect
 * on other events.
 *
 * @{
 */

/* ------------------------------------------------------------------------- */
/** \brief Marks an instantaneous event in the application.
 *
 * A marker can contain a text message or specify additional information
 * using the event attributes structure.  These attributes include a text
 * message, color, category, and a payload. Each of the attributes is optional
 * and can only be sent out using the \ref nvtxMarkEx function.
 * If \ref nvtxMarkA or \ref nvtxMarkW are used to specify the the marker
 * or if an attribute is unspecified then a default value will be used.
 *
 * \param eventAttrib - The event attribute structure defining the marker's
 * attribute types and attribute values.
 *
 * \par Example:
 * \code
 * // zero the structure
 * nvtxEventAttributes_t eventAttrib = {0};
 * // set the version and the size information
 * eventAttrib.version = NVTX_VERSION;
 * eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
 * // configure the attributes.  0 is the default for all attributes.
 * eventAttrib.colorType = NVTX_COLOR_ARGB;
 * eventAttrib.color = 0xFF880000;
 * eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
 * eventAttrib.message.ascii = "Example nvtxMarkEx";
 * nvtxMarkEx(&eventAttrib);
 * \endcode
 *
 * \version \NVTX_VERSION_1
 * @{ */
NVTX_DECLSPEC void NVTX_API nvtxMarkEx(const nvtxEventAttributes_t* eventAttrib);
/** @} */

/* ------------------------------------------------------------------------- */
/** \brief Marks an instantaneous event in the application.
 *
 * A marker created using \ref nvtxMarkA or \ref nvtxMarkW contains only a
 * text message.
 *
 * \param message     - The message associated to this marker event.
 *
 * \par Example:
 * \code
 * nvtxMarkA("Example nvtxMarkA");
 * nvtxMarkW(L"Example nvtxMarkW");
 * \endcode
 *
 * \version \NVTX_VERSION_0
 * @{ */
NVTX_DECLSPEC void NVTX_API nvtxMarkA(const char* message);
NVTX_DECLSPEC void NVTX_API nvtxMarkW(const wchar_t* message);
/** @} */

/** @} */ /* END MARKER_AND_RANGES */

/* ========================================================================= */
/** \name Start/Stop Ranges
 */
/** \addtogroup MARKER_AND_RANGES
 * \section INDEPENDENT_RANGES Start/Stop Ranges
 *
 * Start/Stop ranges denote a time span that can expose arbitrary concurrency -
 * opposed to Push/Pop ranges that only support nesting. In addition the start
 * of a range can happen on a different thread than the end. For the
 * correlation of a start/end pair an unique correlation ID is used that is
 * returned from the start API call and needs to be passed into the end API
 * call.
 *
 * @{
 */

/* ------------------------------------------------------------------------- */
/** \brief Marks the start of a range.
 *
 * \param eventAttrib - The event attribute structure defining the range's
 * attribute types and attribute values.
 *
 * \return The unique ID used to correlate a pair of Start and End events.
 *
 * \remarks Ranges defined by Start/End can overlap.
 *
 * \par Example:
 * \code
 * nvtxEventAttributes_t eventAttrib = {0};
 * eventAttrib.version = NVTX_VERSION;
 * eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
 * eventAttrib.category = 3;
 * eventAttrib.colorType = NVTX_COLOR_ARGB;
 * eventAttrib.color = 0xFF0088FF;
 * eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
 * eventAttrib.message.ascii = "Example RangeStartEnd";
 * nvtxRangeId_t rangeId = nvtxRangeStartEx(&eventAttrib);
 * // ...
 * nvtxRangeEnd(rangeId);
 * \endcode
 *
 * \sa
 * ::nvtxRangeEnd
 *
 * \version \NVTX_VERSION_1
 * @{ */
NVTX_DECLSPEC nvtxRangeId_t NVTX_API nvtxRangeStartEx(const nvtxEventAttributes_t* eventAttrib);
/** @} */

/* ------------------------------------------------------------------------- */
/** \brief Marks the start of a range.
 *
 * \param message     - The event message associated to this range event.
 *
 * \return The unique ID used to correlate a pair of Start and End events.
 *
 * \remarks Ranges defined by Start/End can overlap.
 *
 * \par Example:
 * \code
 * nvtxRangeId_t r1 = nvtxRangeStartA("Range 1");
 * nvtxRangeId_t r2 = nvtxRangeStartW(L"Range 2");
 * nvtxRangeEnd(r1);
 * nvtxRangeEnd(r2);
 * \endcode
 * \sa
 * ::nvtxRangeEnd
 *
 * \version \NVTX_VERSION_0
 * @{ */
NVTX_DECLSPEC nvtxRangeId_t NVTX_API nvtxRangeStartA(const char* message);
NVTX_DECLSPEC nvtxRangeId_t NVTX_API nvtxRangeStartW(const wchar_t* message);
/** @} */

/* ------------------------------------------------------------------------- */
/** \brief Marks the end of a range.
 *
 * \param id - The correlation ID returned from a nvtxRangeStart call.
 *
 * \sa
 * ::nvtxRangeStartEx
 * ::nvtxRangeStartA
 * ::nvtxRangeStartW
 *
 * \version \NVTX_VERSION_0
 * @{ */
NVTX_DECLSPEC void NVTX_API nvtxRangeEnd(nvtxRangeId_t id);
/** @} */

/** @} */


/* ========================================================================= */
/** \name Push/Pop Ranges
 */
/** \addtogroup MARKER_AND_RANGES
 * \section PUSH_POP_RANGES Push/Pop Ranges
 *
 * Push/Pop ranges denote nested time ranges. Nesting is maintained per thread
 * and does not require any additional correlation mechanism. The duration of a
 * push/pop range is defined by the corresponding pair of Push/Pop API calls.
 *
 * @{
 */

/* ------------------------------------------------------------------------- */
/** \brief Marks the start of a nested range
 *
 * \param eventAttrib - The event attribute structure defining the range's
 * attribute types and attribute values.
 *
 * \return The 0 based level of range being started.  If an error occurs a
 * negative value is returned.
 *
 * \par Example:
 * \code
 * nvtxEventAttributes_t eventAttrib = {0};
 * eventAttrib.version = NVTX_VERSION;
 * eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
 * eventAttrib.colorType = NVTX_COLOR_ARGB;
 * eventAttrib.color = 0xFFFF0000;
 * eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
 * eventAttrib.message.ascii = "Level 0";
 * nvtxRangePushEx(&eventAttrib);
 *
 * // Re-use eventAttrib
 * eventAttrib.messageType = NVTX_MESSAGE_TYPE_UNICODE;
 * eventAttrib.message.unicode = L"Level 1";
 * nvtxRangePushEx(&eventAttrib);
 *
 * nvtxRangePop();
 * nvtxRangePop();
 * \endcode
 *
 * \sa
 * ::nvtxRangePop
 *
 * \version \NVTX_VERSION_1
 * @{ */
NVTX_DECLSPEC int NVTX_API nvtxRangePushEx(const nvtxEventAttributes_t* eventAttrib);
/** @} */

/* ------------------------------------------------------------------------- */
/** \brief Marks the start of a nested range
 *
 * \param message     - The event message associated to this range event.
 *
 * \return The 0 based level of range being started.  If an error occurs a
 * negative value is returned.
 *
 * \par Example:
 * \code
 * nvtxRangePushA("Level 0");
 * nvtxRangePushW(L"Level 1");
 * nvtxRangePop();
 * nvtxRangePop();
 * \endcode
 *
 * \sa
 * ::nvtxRangePop
 *
 * \version \NVTX_VERSION_0
 * @{ */
NVTX_DECLSPEC int NVTX_API nvtxRangePushA(const char* message);
NVTX_DECLSPEC int NVTX_API nvtxRangePushW(const wchar_t* message);
/** @} */

/* ------------------------------------------------------------------------- */
/** \brief Marks the end of a nested range
 *
 * \return The level of the range being ended. If an error occurs a negative
 * value is returned on the current thread.
 *
 * \sa
 * ::nvtxRangePushEx
 * ::nvtxRangePushA
 * ::nvtxRangePushW
 *
 * \version \NVTX_VERSION_0
 * @{ */
NVTX_DECLSPEC int NVTX_API nvtxRangePop(void);
/** @} */

/** @} */

/* ========================================================================= */
/** \defgroup RESOURCE_NAMING Resource Naming
 *
 * This section covers calls that allow to annotate objects with user-provided
 * names in order to allow for a better analysis of complex trace data. All of
 * the functions take the handle or the ID of the object to name and the name.
 * The functions can be called multiple times during the execution of an
 * application, however, in that case it is implementation dependent which
 * name will be reported by the tool.
 *
 * \section RESOURCE_NAMING_NVTX NVTX Resource Naming
 * The NVIDIA Tools Extension library allows to attribute events with additional
 * information such as category IDs. These category IDs can be annotated with
 * user-provided names using the respective resource naming functions.
 *
 * \section RESOURCE_NAMING_OS OS Resource Naming
 * In order to enable a tool to report system threads not just by their thread
 * identifier, the NVIDIA Tools Extension library allows to provide user-given
 * names to these OS resources.
 * @{
 */

/* ------------------------------------------------------------------------- */
/** \name Functions for NVTX Resource Naming
 */
/** @{
 * \brief Annotate an NVTX category.
 *
 * Categories are used to group sets of events. Each category is identified
 * through a unique ID and that ID is passed into any of the marker/range
 * events to assign that event to a specific category. The nvtxNameCategory
 * function calls allow the user to assign a name to a category ID.
 *
 * \param category - The category ID to name.
 * \param name     - The name of the category.
 *
 * \remarks The category names are tracked per process.
 *
 * \par Example:
 * \code
 * nvtxNameCategory(1, "Memory Allocation");
 * nvtxNameCategory(2, "Memory Transfer");
 * nvtxNameCategory(3, "Memory Object Lifetime");
 * \endcode
 *
 * \version \NVTX_VERSION_1
 */
NVTX_DECLSPEC void NVTX_API nvtxNameCategoryA(uint32_t category, const char* name);
NVTX_DECLSPEC void NVTX_API nvtxNameCategoryW(uint32_t category, const wchar_t* name);
/** @} */

/* ------------------------------------------------------------------------- */
/** \name Functions for OS Resource Naming
 */
/** @{
 * \brief Annotate an OS thread.
 *
 * Allows the user to name an active thread of the current process. If an
 * invalid thread ID is provided or a thread ID from a different process is
 * used the behavior of the tool is implementation dependent.
 *
 * \param threadId - The ID of the thread to name.
 * \param name     - The name of the thread.
 *
 * \par Example:
 * \code
 * nvtxNameOsThread(GetCurrentThreadId(), "MAIN_THREAD");
 * \endcode
 *
 * \version \NVTX_VERSION_1
 */
NVTX_DECLSPEC void NVTX_API nvtxNameOsThreadA(uint32_t threadId, const char* name);
NVTX_DECLSPEC void NVTX_API nvtxNameOsThreadW(uint32_t threadId, const wchar_t* name);
/** @} */

/** @} */ /* END RESOURCE_NAMING */

/* ========================================================================= */

#ifdef UNICODE
    #define nvtxMark            nvtxMarkW
    #define nvtxRangeStart      nvtxRangeStartW
    #define nvtxRangePush       nvtxRangePushW
    #define nvtxNameCategory    nvtxNameCategoryW
    #define nvtxNameOsThread    nvtxNameOsThreadW
#else
    #define nvtxMark            nvtxMarkA
    #define nvtxRangeStart      nvtxRangeStartA
    #define nvtxRangePush       nvtxRangePushA
    #define nvtxNameCategory    nvtxNameCategoryA
    #define nvtxNameOsThread    nvtxNameOsThreadA
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVTOOLSEXT_H_ */
