/* $Id$

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "hb.h"
#include "hbffmpeg.h"
#include "lang.h"
#include "a52dec/a52.h"
#include "mp4v2/mp4v2.h"
#include "libbluray/bluray.h"

#define min(a, b) a < b ? a : b
#define HB_MAX_PROBE_SIZE (1*1024*1024)

/*
 * This table defines how ISO MPEG stream type codes map to HandBrake
 * codecs. It is indexed by the 8 bit stream type and contains the codec
 * worker object id and a parameter for that worker proc (ignored except
 * for the ffmpeg-based codecs in which case it is the ffmpeg codec id).
 *
 * Entries with a worker proc id of 0 or a kind of 'U' indicate that HB
 * doesn't handle the stream type.
 * N - Not used
 * U - Unknown (to be determined by further processing)
 * A - Audio
 * V - Video
 * P - PCR
 */
typedef enum { N, U, A, V, P } kind_t;
typedef struct {
    kind_t kind; /* not handled / unknown / audio / video */
    int codec;          /* HB worker object id of codec */
    int codec_param;    /* param for codec (usually ffmpeg codec id) */
    const char* name;   /* description of type */
} stream2codec_t;

#define st(id, kind, codec, codec_param, name) \
 [id] = { kind, codec, codec_param, name }

static const stream2codec_t st2codec[256] = {
    st(0x00, U, 0,                 0,              NULL),
    st(0x01, V, WORK_DECMPEG2,     0,              "MPEG1"),
    st(0x02, V, WORK_DECMPEG2,     0,              "MPEG2"),
    st(0x03, A, HB_ACODEC_FFMPEG,  CODEC_ID_MP2,   "MPEG1"),
    st(0x04, A, HB_ACODEC_FFMPEG,  CODEC_ID_MP2,   "MPEG2"),
    st(0x05, N, 0,                 0,              "ISO 13818-1 private section"),
    st(0x06, U, 0,                 0,              "ISO 13818-1 PES private data"),
    st(0x07, N, 0,                 0,              "ISO 13522 MHEG"),
    st(0x08, N, 0,                 0,              "ISO 13818-1 DSM-CC"),
    st(0x09, N, 0,                 0,              "ISO 13818-1 auxiliary"),
    st(0x0a, N, 0,                 0,              "ISO 13818-6 encap"),
    st(0x0b, N, 0,                 0,              "ISO 13818-6 DSM-CC U-N msgs"),
    st(0x0c, N, 0,                 0,              "ISO 13818-6 Stream descriptors"),
    st(0x0d, N, 0,                 0,              "ISO 13818-6 Sections"),
    st(0x0e, N, 0,                 0,              "ISO 13818-1 auxiliary"),
    st(0x0f, A, HB_ACODEC_FFAAC,   CODEC_ID_AAC,   "AAC"),
    st(0x10, V, WORK_DECAVCODECV,  CODEC_ID_MPEG4, "MPEG4"),
    st(0x11, A, HB_ACODEC_FFMPEG,  CODEC_ID_AAC_LATM, "LATM AAC"),
    st(0x12, U, 0,                 0,              "MPEG4 generic"),

    st(0x14, N, 0,                 0,              "ISO 13818-6 DSM-CC download"),

    st(0x1b, V, WORK_DECAVCODECV,  CODEC_ID_H264,  "H.264"),

    st(0x80, U, HB_ACODEC_FFMPEG,  CODEC_ID_PCM_BLURAY, "Digicipher II Video"),
    st(0x81, A, HB_ACODEC_AC3,     0,              "AC3"),
    st(0x82, A, HB_ACODEC_DCA,     0,              "DTS"),
    // 0x83 can be LPCM or BD TrueHD.  Set to 'unknown' till we know more.
    st(0x83, U, HB_ACODEC_LPCM,    0,              "LPCM"),
    // BD E-AC3 Primary audio
    st(0x84, U, 0,                 0,              "SDDS"),
    st(0x85, U, 0,                 0,              "ATSC Program ID"),
    // 0x86 can be BD DTS-HD/DTS. Set to 'unknown' till we know more.
    st(0x86, U, HB_ACODEC_DCA_HD,  CODEC_ID_DTS,   "DTS-HD MA"),
    st(0x87, A, HB_ACODEC_FFMPEG,  CODEC_ID_EAC3,  "E-AC3"),

    st(0x8a, A, HB_ACODEC_DCA,     0,              "DTS"),

    st(0x90, N, 0,                 0,              "PGS Subtitle"),
    // 0x91 can be AC3 or BD Interactive Graphics Stream.
    st(0x91, U, 0,                 0,              "AC3/IGS"),
    st(0x92, N, 0,                 0,              "Subtitle"),

    st(0x94, U, 0,                 0,              "SDDS"),
    st(0xa0, V, 0,                 0,              "MSCODEC"),
    // BD E-AC3 Secondary audio
    st(0xa1, U, 0,                 0,              "E-AC3"),
    // BD DTS-HD Secondary audio
    st(0xa2, U, 0,                 0,              "DTS-HD LBR"),

    st(0xea, V, WORK_DECAVCODECV,  CODEC_ID_VC1,   "VC-1"),
};
#undef st

typedef enum {
    hb_stream_type_unknown = 0,
    transport,
    program,
    ffmpeg
} hb_stream_type_t;

#define MAX_PS_PROBE_SIZE (5*1024*1024)
#define kMaxNumberPMTStreams 32

typedef struct {
    hb_buffer_t *buf;
    hb_buffer_t *extra_buf;
    int8_t  skipbad;
    int8_t  continuity;
    uint8_t pkt_summary[8];
    int     pid;
    uint8_t is_pcr;
    int     pes_list;

} hb_ts_stream_t;

typedef struct {
    int      map_idx;
    int      stream_id;
    uint8_t  stream_id_ext;
    uint8_t  stream_type;
    kind_t   stream_kind;
    int      lang_code;
    uint32_t format_id;
#define TS_FORMAT_ID_AC3 (('A' << 24) | ('C' << 16) | ('-' << 8) | '3')
    int      codec;         // HB worker object id of codec
    int      codec_param;   // param for codec (usually ffmpeg codec id)
    char     codec_name[80];
    int      next;          // next pointer for list
                            // hb_ts_stream_t points to a list of
                            // hb_pes_stream_t
    hb_buffer_t  *probe_buf;
    int      probe_next_size;
} hb_pes_stream_t;

struct hb_stream_s
{
    int     scan;
    int     frames;             /* video frames so far */
    int     errors;             /* total errors so far */
    int     last_error_frame;   /* frame # at last error message */
    int     last_error_count;   /* # errors at last error message */
    int     packetsize;         /* Transport Stream packet size */

    int     need_keyframe;      // non-zero if want to start at a keyframe

    int      chapter;           /* Chapter that we are currently in */
    int64_t  chapter_end;       /* HB time that the current chapter ends */


    struct
    {
        uint8_t found_pcr;      // non-zero if we've found at least one pcr
        int     pcr_out;        // sequence number of most recent output pcr
        int     pcr_in;         // sequence number of most recent input pcr
        int     pcr_discontinuity; // sequence number of last discontinuity
        int     pcr_current;    // last discontinuity sent to reader
        int64_t pcr;            // most recent input pcr
        int64_t last_timestamp; // used for discontinuity detection when
                                // there are no PCRs

        uint8_t *packet;        // buffer for one TS packet
        hb_ts_stream_t *list;
        int count;
        int alloc;
    } ts;

    struct
    {
        uint8_t found_scr;      // non-zero if we've found at least one scr
        int64_t scr;            // most recent input scr
        hb_pes_stream_t *list;
        int count;
        int alloc;
    } pes;

    /*
     * Stuff before this point is dynamic state updated as we read the
     * stream. Stuff after this point is stream description state that
     * we learn during the initial scan but cache so it can be
     * reused during the conversion read.
     */
    uint8_t has_IDRs;           // # IDRs found during duration scan
    uint8_t ts_flags;           // stream characteristics:
#define         TS_HAS_PCR  (1 << 0)    // at least one PCR seen
#define         TS_HAS_RAP  (1 << 1)    // Random Access Point bit seen
#define         TS_HAS_RSEI (1 << 2)    // "Restart point" SEI seen

    char    *path;
    FILE    *file_handle;
    hb_stream_type_t hb_stream_type;
    hb_title_t *title;

    AVFormatContext *ffmpeg_ic;
    AVPacket *ffmpeg_pkt;
    uint8_t ffmpeg_video_id;

    uint32_t reg_desc;          // 4 byte registration code that identifies
                                // stream semantics

    struct
    {
        unsigned short program_number;
        unsigned short program_map_PID;
    } pat_info[kMaxNumberPMTStreams];
    int     ts_number_pat_entries;

    struct
    {
        int reading;
        unsigned char *tablebuf;
        unsigned int tablepos;
        unsigned char current_continuity_counter;

        unsigned int PCR_PID;
    } pmt_info;
};

typedef struct {
    uint8_t *buf;
    uint32_t val;
    int pos;
    int size;
} bitbuf_t;

typedef struct
{
    uint8_t has_stream_id_ext;
    uint8_t stream_id;
    uint8_t stream_id_ext;
    uint8_t bd_substream_id;
    int64_t pts;
    int64_t dts;
    int64_t scr;
    int     header_len;
    int     packet_len;
} hb_pes_info_t;


/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle);
static off_t align_to_next_packet(hb_stream_t *stream);
static int64_t pes_timestamp( const uint8_t *pes );

static void hb_ts_stream_init(hb_stream_t *stream);
static hb_buffer_t * hb_ts_stream_decode(hb_stream_t *stream);
static void hb_init_audio_list(hb_stream_t *stream, hb_title_t *title);
static void hb_ts_stream_find_pids(hb_stream_t *stream);

static void hb_ps_stream_init(hb_stream_t *stream);
static hb_buffer_t * hb_ps_stream_decode(hb_stream_t *stream);
static void hb_ps_stream_find_streams(hb_stream_t *stream);
static int hb_ps_read_packet( hb_stream_t * stream, hb_buffer_t *b );
static int update_ps_streams( hb_stream_t * stream, int stream_id, int stream_id_ext, int stream_type, int in_kind );
static int update_ts_streams( hb_stream_t * stream, int pid, int stream_id_ext, int stream_type, int in_kind, int *pes_idx );
static void update_pes_kind( hb_stream_t * stream, int idx );

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title, int scan );
static void ffmpeg_close( hb_stream_t *d );
static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream, hb_title_t *title );
hb_buffer_t *hb_ffmpeg_read( hb_stream_t *stream );
static int ffmpeg_seek( hb_stream_t *stream, float frac );
static int ffmpeg_seek_ts( hb_stream_t *stream, int64_t ts );
static inline unsigned int bits_get(bitbuf_t *bb, int bits);
static inline void bits_init(bitbuf_t *bb, uint8_t* buf, int bufsize, int clear);
static inline unsigned int bits_peek(bitbuf_t *bb, int bits);
static inline int bits_eob(bitbuf_t *bb);
static inline int bits_read_ue(bitbuf_t *bb );
static void pes_add_audio_to_title(hb_stream_t *s, int i, hb_title_t *t, int sort);
static int hb_parse_ps( hb_stream_t *stream, uint8_t *buf, int len, hb_pes_info_t *pes_info );
static void hb_ts_resolve_pid_types(hb_stream_t *stream);
static void hb_ps_resolve_stream_types(hb_stream_t *stream);
void hb_ts_stream_reset(hb_stream_t *stream);
void hb_ps_stream_reset(hb_stream_t *stream);

/*
 * logging routines.
 * these frontend hb_log because transport streams can have a lot of errors
 * so we want to rate limit messages. this routine limits the number of
 * messages to at most one per minute of video. other errors that occur
 * during the minute are counted & the count is output with the next
 * error msg we print.
 */
static void ts_warn_helper( hb_stream_t *stream, char *log, va_list args )
{
    // limit error printing to at most one per minute of video (at 30fps)
    ++stream->errors;
    if ( stream->frames - stream->last_error_frame >= 30*60 )
    {
        char msg[256];

        vsnprintf( msg, sizeof(msg), log, args );

        if ( stream->errors - stream->last_error_count < 10 )
        {
            hb_log( "stream: error near frame %d: %s", stream->frames, msg );
        }
        else
        {
            int Edelta = stream->errors - stream->last_error_count;
            double Epcnt = (double)Edelta * 100. /
                            (stream->frames - stream->last_error_frame);
            hb_log( "stream: %d new errors (%.0f%%) up to frame %d: %s",
                    Edelta, Epcnt, stream->frames, msg );
        }
        stream->last_error_frame = stream->frames;
        stream->last_error_count = stream->errors;
    }
}

static void ts_warn( hb_stream_t*, char*, ... ) HB_WPRINTF(2,3);
static void ts_err( hb_stream_t*, int, char*, ... ) HB_WPRINTF(3,4);

static void ts_warn( hb_stream_t *stream, char *log, ... )
{
    va_list args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );
}

static int get_id(hb_pes_stream_t *pes)
{
    return ( pes->stream_id_ext << 16 ) + pes->stream_id;
}

static int index_of_id(hb_stream_t *stream, int id)
{
    int i;

    for ( i = 0; i < stream->pes.count; ++i )
    {
        if ( id == get_id( &stream->pes.list[i] ) )
            return i;
    }

    return -1;
}

static int index_of_pid(hb_stream_t *stream, int pid)
{
    int i;

    for ( i = 0; i < stream->ts.count; ++i )
    {
        if ( pid == stream->ts.list[i].pid )
        {
            return i;
        }
    }

    return -1;
}

static int index_of_ps_stream(hb_stream_t *stream, int id, int sid)
{
    int i;

    for ( i = 0; i < stream->pes.count; ++i )
    {
        if ( id == stream->pes.list[i].stream_id &&
             sid == stream->pes.list[i].stream_id_ext )
        {
            return i;
        }
    }
    // If there is no match on the stream_id_ext, try matching 
    // on only the stream_id.
    for ( i = 0; i < stream->pes.count; ++i )
    {
        if ( id == stream->pes.list[i].stream_id &&
             0 == stream->pes.list[i].stream_id_ext )
        {
            return i;
        }
    }

    return -1;
}

static kind_t ts_stream_kind( hb_stream_t * stream, int idx )
{
    if ( stream->ts.list[idx].pes_list != -1 )
    {
        // Retuns kind for the first pes substream in the pes list
        // All substreams in a TS stream are the same kind.
        return stream->pes.list[stream->ts.list[idx].pes_list].stream_kind;
    }
    else
    {
        return U;
    }
}

static kind_t ts_stream_type( hb_stream_t * stream, int idx )
{
    if ( stream->ts.list[idx].pes_list != -1 )
    {
        // Retuns stream type for the first pes substream in the pes list
        // All substreams in a TS stream are the same stream type.
        return stream->pes.list[stream->ts.list[idx].pes_list].stream_type;
    }
    else
    {
        return 0x00;
    }
}

static int pes_index_of_video(hb_stream_t *stream)
{
    int i;

    for ( i = 0; i < stream->pes.count; ++i )
        if ( V == stream->pes.list[i].stream_kind )
            return i;

    return -1;
}

static int ts_index_of_video(hb_stream_t *stream)
{
    int i;

    for ( i = 0; i < stream->ts.count; ++i )
        if ( V == ts_stream_kind( stream, i ) )
            return i;

    return -1;
}

static void ts_err( hb_stream_t *stream, int curstream, char *log, ... )
{
    va_list args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );

    stream->ts.list[curstream].skipbad = 1;
    stream->ts.list[curstream].continuity = -1;
}

static int check_ps_sync(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack header in the
    // first four bytes.
    return (buf[0] == 0x00) && (buf[1] == 0x00) &&
           (buf[2] == 0x01) && (buf[3] == 0xba);
}

static int check_ps_sc(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack followed by a
    // some other start code. If we've already verified the pack, this skip
    // it and checks for a start code prefix.
    int pos;
    int mark = buf[4] >> 4;
    if ( mark == 0x02 )
    {
        // mpeg-1 pack header
        pos = 12;   // skip over the PACK
    }
    else
    {
        // mpeg-2 pack header
        pos = 14 + ( buf[13] & 0x7 );   // skip over the PACK
    }
    return (buf[pos+0] == 0x00) && (buf[pos+1] == 0x00) && (buf[pos+2] == 0x01);
}

static int check_ts_sync(const uint8_t *buf)
{
    // must have initial sync byte & a legal adaptation ctrl
    return (buf[0] == 0x47) && (((buf[3] & 0x30) >> 4) > 0);
}

static int have_ts_sync(const uint8_t *buf, int psize, int count)
{
    int ii;
    for ( ii = 0; ii < count; ii++ )
    {
        if ( !check_ts_sync(&buf[ii*psize]) )
            return 0;
    }
    return 1;
}

static int hb_stream_check_for_ts(const uint8_t *buf)
{
    // transport streams should have a sync byte every 188 bytes.
    // search the first 8KB of buf looking for at least 8 consecutive
    // correctly located sync patterns.
    int offset = 0;
    int count = 16;

    for ( offset = 0; offset < 8*1024-count*188; ++offset )
    {
        if ( have_ts_sync( &buf[offset], 188, count) )
            return 188 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 192, count) )
            return 192 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 204, count) )
            return 204 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 208, count) )
            return 208 | (offset << 8);
    }
    return 0;
}

static int hb_stream_check_for_ps(hb_stream_t *stream)
{
    uint8_t buf[2048*4];
    uint8_t sc_buf[4];
    int pos = 0;

    fseek(stream->file_handle, 0, SEEK_SET);

    // program streams should start with a PACK then some other mpeg start
    // code (usually a SYS but that might be missing if we only have a clip).
    while (pos < 512 * 1024)
    {
        int offset;

        if ( fread(buf, 1, sizeof(buf), stream->file_handle) != sizeof(buf) )
            return 0;

        for ( offset = 0; offset < 8*1024-27; ++offset )
        {
            if ( check_ps_sync( &buf[offset] ) && check_ps_sc( &buf[offset] ) )
            {
                int pes_offset, prev, data_len;
                uint8_t sid;
                uint8_t *b = buf+offset;

                // Skip the pack header
                int mark = buf[4] >> 4;
                if ( mark == 0x02 )
                {
                    // mpeg-1 pack header
                    pes_offset = 12;
                }
                else
                {
                    // mpeg-2 pack header
                    pes_offset = 14 + ( buf[13] & 0x7 );
                }

                b +=  pes_offset;
                // Get the next stream id
                sid = b[3];
                data_len = (b[4] << 8) + b[5];
                if ( data_len && sid > 0xba && sid < 0xf9 )
                {
                    prev = ftell( stream->file_handle );
                    pos = prev - ( sizeof(buf) - offset );
                    pos += pes_offset + 6 + data_len;
                    fseek( stream->file_handle, pos, SEEK_SET );
                    if ( fread(sc_buf, 1, 4, stream->file_handle) != 4 )
                        return 0;
                    if (sc_buf[0] == 0x00 && sc_buf[1] == 0x00 &&
                        sc_buf[2] == 0x01)
                    {
                        return 1;
                    }
                    fseek( stream->file_handle, prev, SEEK_SET );
                }
            }
        }
        fseek( stream->file_handle, -27, SEEK_CUR );
        pos = ftell( stream->file_handle );
    }
    return 0;
}

static int hb_stream_get_type(hb_stream_t *stream)
{
    uint8_t buf[2048*4];

    if ( fread(buf, 1, sizeof(buf), stream->file_handle) == sizeof(buf) )
    {
        int psize;
        if ( ( psize = hb_stream_check_for_ts(buf) ) != 0 )
        {
            int offset = psize >> 8;
            psize &= 0xff;
            hb_log("file is MPEG Transport Stream with %d byte packets"
                   " offset %d bytes", psize, offset);
            stream->packetsize = psize;
            stream->hb_stream_type = transport;
            hb_ts_stream_init(stream);
            return 1;
        }
        if ( hb_stream_check_for_ps(stream) != 0 )
        {
            hb_log("file is MPEG Program Stream");
            stream->hb_stream_type = program;
            hb_ps_stream_init(stream);
            // We default to mpeg codec for ps streams if no
            // video found in program stream map
            return 1;
        }
    }
    return 0;
}

static void hb_stream_delete_dynamic( hb_stream_t *d )
{
    if( d->file_handle )
    {
        fclose( d->file_handle );
        d->file_handle = NULL;
    }

    int i=0;

    if ( d->ts.packet )
    {
        free( d->ts.packet );
        d->ts.packet = NULL;
    }
    if ( d->ts.list )
    {
        for (i = 0; i < d->ts.count; i++)
        {
            if (d->ts.list[i].buf)
            {
                hb_buffer_close(&(d->ts.list[i].buf));
                hb_buffer_close(&(d->ts.list[i].extra_buf));
                d->ts.list[i].buf = NULL;
                d->ts.list[i].extra_buf = NULL;
            }
        }
    }
}

static void hb_stream_delete( hb_stream_t *d )
{
    hb_stream_delete_dynamic( d );
    free( d->ts.list );
    free( d->pes.list );
    free( d->path );
    free( d );
}

static int audio_inactive( hb_stream_t *stream, int id, int stream_id_ext )
{
    if ( id < 0 )
    {
        // PID declared inactive by hb_stream_title_scan
        return 1;
    }
    if ( id == stream->pmt_info.PCR_PID )
    {
        // PCR PID is always active
        return 0;
    }

    int i;
    for ( i = 0; i < hb_list_count( stream->title->list_audio ); ++i )
    {
        hb_audio_t *audio = hb_list_item( stream->title->list_audio, i );
        if ( audio->id == ((stream_id_ext << 16) | id) )
        {
            return 0;
        }
    }
    return 1;
}

/* when the file was first opened we made entries for all the audio elementary
 * streams we found in it. Streams that were later found during the preview scan
 * now have an audio codec, type, rate, etc., associated with them. At the end
 * of the scan we delete all the audio entries that weren't found by the scan
 * or don't have a format we support. This routine deletes audio entry 'indx'
 * by setting its PID to an invalid value so no packet will match it. (We can't
 * move any of the entries since the index of the entry is used as the id
 * of the media stream for HB. */
static void hb_stream_delete_ts_entry(hb_stream_t *stream, int indx)
{
    if ( stream->ts.list[indx].pid > 0 )
    {
        stream->ts.list[indx].pid = -stream->ts.list[indx].pid;
    }
}

static int hb_stream_try_delete_ts_entry(hb_stream_t *stream, int indx)
{
    int ii;

    if ( stream->ts.list[indx].pid < 0 )
        return 1;

    for ( ii = stream->ts.list[indx].pes_list; ii != -1;
          ii = stream->pes.list[ii].next )
    {
        if ( stream->pes.list[ii].stream_id >= 0 )
            return 0;
    }
    stream->ts.list[indx].pid = -stream->ts.list[indx].pid;
    return 1;
}

static void hb_stream_delete_ps_entry(hb_stream_t *stream, int indx)
{
    if ( stream->pes.list[indx].stream_id > 0 )
    {
        stream->pes.list[indx].stream_id = -stream->pes.list[indx].stream_id;
    }
}

static void prune_streams(hb_stream_t *d)
{
    if ( d->hb_stream_type == transport )
    {
        int ii, jj;
        for ( ii = 0; ii < d->ts.count; ii++)
        {
            // If probing didn't find audio or video, and the pid
            // is not the PCR, remove the track
            if ( ts_stream_kind ( d, ii ) == U &&
                 !d->ts.list[ii].is_pcr )
            {
                hb_stream_delete_ts_entry(d, ii);
                continue;
            }

            if ( ts_stream_kind ( d, ii ) == A )
            {
                for ( jj = d->ts.list[ii].pes_list; jj != -1;
                      jj = d->pes.list[jj].next )
                {
                    if ( audio_inactive( d, d->pes.list[jj].stream_id,
                                         d->pes.list[jj].stream_id_ext ) )
                    {
                        hb_stream_delete_ps_entry(d, jj);
                    }
                }
                if ( !d->ts.list[ii].is_pcr &&
                     hb_stream_try_delete_ts_entry(d, ii) )
                {
                    continue;
                }
            }
        }
        // reset to beginning of file and reset some stream 
        // state information
        hb_stream_seek( d, 0. );
    }
    else if ( d->hb_stream_type == program )
    {
        int ii;
        for ( ii = 0; ii < d->pes.count; ii++)
        {
            // If probing didn't find audio or video, remove the track
            if ( d->pes.list[ii].stream_kind == U )
            {
                hb_stream_delete_ps_entry(d, ii);
            }

            if ( d->pes.list[ii].stream_kind == A &&
                 audio_inactive( d, d->pes.list[ii].stream_id,
                                 d->pes.list[ii].stream_id_ext ) )
            {
                // this PID isn't wanted (we don't have a codec for it
                // or scan didn't find audio parameters)
                hb_stream_delete_ps_entry(d, ii);
                continue;
            }
        }
        // reset to beginning of file and reset some stream 
        // state information
        hb_stream_seek( d, 0. );
    }
}

/***********************************************************************
 * hb_stream_open
 ***********************************************************************
 *
 **********************************************************************/
hb_stream_t * hb_stream_open( char *path, hb_title_t *title, int scan )
{
    FILE *f = fopen( path, "rb" );
    if ( f == NULL )
    {
        hb_log( "hb_stream_open: open %s failed", path );
        return NULL;
    }

    hb_stream_t *d = calloc( sizeof( hb_stream_t ), 1 );
    if ( d == NULL )
    {
        fclose( f );
        hb_log( "hb_stream_open: can't allocate space for %s stream state", path );
        return NULL;
    }

    if( title && !( title->flags & HBTF_NO_IDR ) )
    {
        d->has_IDRs = 1;
    }

    /*
     * If it's something we can deal with (MPEG2 PS or TS) return a stream
     * reference structure & null otherwise.
     */
    d->file_handle = f;
    d->title = title;
    d->scan = scan;
    d->path = strdup( path );
    if (d->path != NULL )
    {
        if ( hb_stream_get_type( d ) != 0 )
        {
            if( !scan )
            {
                prune_streams( d );
            }
            // reset to beginning of file and reset some stream 
            // state information
            hb_stream_seek( d, 0. );
            return d;
        }
        fclose( d->file_handle );
        d->file_handle = NULL;
        if ( ffmpeg_open( d, title, scan ) )
        {
            return d;
        }
    }
    if ( d->file_handle )
    {
        fclose( d->file_handle );
    }
    if (d->path)
    {
        free( d->path );
    }
    hb_log( "hb_stream_open: open %s failed", path );
    free( d );
    return NULL;
}

static int new_pid( hb_stream_t * stream )
{
    int num = stream->ts.alloc;

    if ( stream->ts.count == stream->ts.alloc )
    {
        num = stream->ts.alloc ? stream->ts.alloc * 2 : 32;
        stream->ts.list = realloc( stream->ts.list,
                                   sizeof( hb_ts_stream_t ) * num );
    }
    int ii;
    for ( ii = stream->ts.alloc; ii < num; ii++ )
    {
        memset(&stream->ts.list[ii], 0, sizeof( hb_ts_stream_t ));
        stream->ts.list[ii].continuity = -1;
        stream->ts.list[ii].pid = -1;
        stream->ts.list[ii].pes_list = -1;
    }
    stream->ts.alloc = num;
    num = stream->ts.count;
    stream->ts.count++;

    return num;
}

static int new_pes( hb_stream_t * stream )
{
    int num = stream->pes.alloc;

    if ( stream->pes.count == stream->pes.alloc )
    {
        num = stream->pes.alloc ? stream->pes.alloc * 2 : 32;
        stream->pes.list = realloc( stream->pes.list,
                                    sizeof( hb_pes_stream_t ) * num );
    }
    int ii;
    for ( ii = stream->pes.alloc; ii < num; ii++ )
    {
        memset(&stream->pes.list[ii], 0, sizeof( hb_pes_stream_t ));
        stream->pes.list[ii].stream_id = -1;
        stream->pes.list[ii].next = -1;
    }
    stream->pes.alloc = num;
    num = stream->pes.count;
    stream->pes.count++;

    return num;
}

hb_stream_t * hb_bd_stream_open( hb_title_t *title )
{
    int ii;

    hb_stream_t *d = calloc( sizeof( hb_stream_t ), 1 );
    if ( d == NULL )
    {
        hb_error( "hb_bd_stream_open: can't allocate space for stream state" );
        return NULL;
    }

    d->file_handle = NULL;
    d->title = title;
    d->path = NULL;
    d->ts.packet = NULL;

    int pid = title->video_id;
    int stream_type = title->video_stream_type;
    update_ts_streams( d, pid, 0, stream_type, V, NULL );

    hb_audio_t * audio;
    for ( ii = 0; ( audio = hb_list_item( title->list_audio, ii ) ); ++ii )
    {
        int stream_id_ext = audio->config.in.substream_type;
        pid = audio->id & 0xFFFF;
        stream_type = audio->config.in.stream_type;

        update_ts_streams( d, pid, stream_id_ext, stream_type, A, NULL );
    }

    // When scanning, title->job == NULL.  We don't need to wait for
    // a PCR when scanning. In fact, it trips us up on the first
    // preview of every title since we would have to read quite a
    // lot of data before finding the PCR.
    if ( title->job )
    {
        /* BD has PCRs, but the BD index always points to a packet
         * after a PCR packet, so we will not see the initial PCR
         * after any seek.  So don't set the flag that causes us
         * to drop packets till we see a PCR. */
        //d->ts_flags = TS_HAS_RAP | TS_HAS_PCR;

        // BD PCR PID is specified to always be 0x1001
        update_ts_streams( d, 0x1001, 0, -1, P, NULL );
    }

    d->packetsize = 192;
    d->hb_stream_type = transport;

    for ( ii = 0; ii < d->ts.count; ii++ )
    {
        d->ts.list[ii].buf = hb_buffer_init(d->packetsize);
        d->ts.list[ii].extra_buf = hb_buffer_init(d->packetsize);
        d->ts.list[ii].buf->size = 0;
        d->ts.list[ii].extra_buf->size = 0;
    }

    return d;
}

/***********************************************************************
 * hb_stream_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_stream_close( hb_stream_t ** _d )
{
    hb_stream_t *stream = * _d;

    if ( stream->hb_stream_type == ffmpeg )
    {
        ffmpeg_close( stream );
        hb_stream_delete( stream );
        *_d = NULL;
        return;
    }

    if ( stream->frames )
    {
        hb_log( "stream: %d good frames, %d errors (%.0f%%)", stream->frames,
                stream->errors, (double)stream->errors * 100. /
                (double)stream->frames );
    }

    hb_stream_delete( stream );
    *_d = NULL;
}

/***********************************************************************
 * hb_ps_stream_title_scan
 ***********************************************************************
 *
 **********************************************************************/
hb_title_t * hb_stream_title_scan(hb_stream_t *stream, hb_title_t * title)
{
    if ( stream->hb_stream_type == ffmpeg )
        return ffmpeg_title_scan( stream, title );

    // 'Barebones Title'
    title->type = HB_STREAM_TYPE;
    title->index = 1;

    // Copy part of the stream path to the title name
    char *sep = strrchr(stream->path, '/');
    if (sep)
        strcpy(title->name, sep+1);
    char *dot_term = strrchr(title->name, '.');
    if (dot_term)
        *dot_term = '\0';

    // Figure out how many audio streams we really have:
    // - For transport streams, for each PID listed in the PMT (whether
    //   or not it was an audio stream type) read the bitstream until we
    //   find an packet from that PID containing a PES header and see if
    //   the elementary stream is an audio type.
    // - For program streams read the first 4MB and take every unique
    //   audio stream we find.
    hb_init_audio_list(stream, title);

    // set the video id, codec & muxer
    int idx = pes_index_of_video( stream );
    if ( idx < 0 )
    {
        hb_title_close( &title );
        return NULL;
    }

    title->video_id = get_id( &stream->pes.list[idx] );
    title->video_codec = stream->pes.list[idx].codec;
    title->video_codec_param = stream->pes.list[idx].codec_param;
    title->demuxer = HB_MPEG_DEMUXER;

    if (stream->hb_stream_type == transport)
    {

        // make sure we're grabbing the PCR PID
        update_ts_streams( stream, stream->pmt_info.PCR_PID, 0, -1, P, NULL );
    }

    // IDRs will be search for in hb_stream_duration
    stream->has_IDRs = 0;
    hb_stream_duration(stream, title);

    // One Chapter
    hb_chapter_t * chapter;
    chapter = calloc( sizeof( hb_chapter_t ), 1 );
    chapter->index = 1;
    chapter->duration = title->duration;
    chapter->hours = title->hours;
    chapter->minutes = title->minutes;
    chapter->seconds = title->seconds;
    hb_list_add( title->list_chapter, chapter );


    if ( stream->has_IDRs < 1 )
    {
        hb_log( "stream doesn't seem to have video IDR frames" );
        title->flags |= HBTF_NO_IDR;
    }

    if ( stream->hb_stream_type == transport && 
         ( stream->ts_flags & TS_HAS_PCR ) == 0 )
    {
        hb_log( "transport stream missing PCRs - using video DTS instead" );
    }

    // Height, width, rate and aspect ratio information is filled in
    // when the previews are built
    return title;
}

/*
 * read the next transport stream packet from 'stream'. Return NULL if
 * we hit eof & a pointer to the sync byte otherwise.
 */
static const uint8_t *next_packet( hb_stream_t *stream )
{
    uint8_t *buf = stream->ts.packet + stream->packetsize - 188;

    while ( 1 )
    {
        if ( fread(stream->ts.packet, 1, stream->packetsize, stream->file_handle) !=
             stream->packetsize )
        {
            return NULL;
        }
        if (buf[0] == 0x47)
        {
            return buf;
        }
        // lost sync - back up to where we started then try to re-establish.
        off_t pos = ftello(stream->file_handle) - stream->packetsize;
        off_t pos2 = align_to_next_packet(stream);
        if ( pos2 == 0 )
        {
            hb_log( "next_packet: eof while re-establishing sync @ %"PRId64, pos );
            return NULL;
        }
        ts_warn( stream, "next_packet: sync lost @ %"PRId64", regained after %"PRId64" bytes",
                 pos, pos2 );
    }
}

/*
 * skip to the start of the next PACK header in program stream src_stream.
 */
static void skip_to_next_pack( hb_stream_t *src_stream )
{
    // scan forward until we find the start of the next pack
    uint32_t strt_code = -1;
    int c;

    flockfile( src_stream->file_handle );
    while ( ( c = getc_unlocked( src_stream->file_handle ) ) != EOF )
    {
        strt_code = ( strt_code << 8 ) | c;
        if ( strt_code == 0x000001ba )
            // we found the start of the next pack
            break;
    }
    funlockfile( src_stream->file_handle );

    // if we didn't terminate on an eof back up so the next read
    // starts on the pack boundary.
    if ( c != EOF )
    {
        fseeko( src_stream->file_handle, -4, SEEK_CUR );
    }
}

static void CreateDecodedNAL( uint8_t **dst, int *dst_len,
                              const uint8_t *src, int src_len )
{
    const uint8_t *end = &src[src_len];
    uint8_t *d = malloc( src_len );

    *dst = d;

    if( d )
    {
        while( src < end )
        {
            if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
                src[2] == 0x01 )
            {
                // Next start code found
                break;
            }
            if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
                src[2] == 0x03 )
            {
                *d++ = 0x00;
                *d++ = 0x00;

                src += 3;
                continue;
            }
            *d++ = *src++;
        }
    }
    *dst_len = d - *dst;
}

static int isRecoveryPoint( const uint8_t *buf, int len )
{
    uint8_t *nal;
    int nal_len;
    int ii, type, size, start;
    int recovery_frames = 0;

    CreateDecodedNAL( &nal, &nal_len, buf, len );

    for ( ii = 0; ii+1 < nal_len; )
    {
        start = ii;
        type = 0;
        while ( ii+1 < nal_len )
        {
            type += nal[ii++];
            if ( nal[ii-1] != 0xff )
                break;
        }
        size = 0;
        while ( ii+1 < nal_len )
        {
            size += nal[ii++];
            if ( nal[ii-1] != 0xff )
                break;
        }

        if( type == 6 )
        {
            recovery_frames = 1;
            break;
        }
        ii += size;
    }

    free( nal );
    return recovery_frames;
}

static int isIframe( hb_stream_t *stream, const uint8_t *buf, int len )
{
    // For mpeg2: look for a gop start or i-frame picture start
    // for h.264: look for idr nal type or a slice header for an i-frame
    // for vc1:   look for a Sequence header
    int ii;
    uint32_t strid = 0;


    int vid = pes_index_of_video( stream );
    hb_pes_stream_t *pes = &stream->pes.list[vid];
    if ( pes->stream_type <= 2 ||
         pes->codec_param == CODEC_ID_MPEG1VIDEO ||
         pes->codec_param == CODEC_ID_MPEG2VIDEO )
    {
        // This section of the code handles MPEG-1 and MPEG-2 video streams
        for (ii = 0; ii < len; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code
                uint8_t id = strid;
                switch ( id )
                {
                    case 0xB8: // group_start_code (GOP header)
                    case 0xB3: // sequence_header code
                        return 1;

                    case 0x00: // picture_start_code
                        // picture_header, let's see if it's an I-frame
                        if (ii < len - 3)
                        {
                            // check if picture_coding_type == 1
                            if ((buf[ii+2] & (0x7 << 3)) == (1 << 3))
                            {
                                // found an I-frame picture
                                return 1;
                            }
                        }
                        break;
                }
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( pes->stream_type == 0x1b || pes->codec_param == CODEC_ID_H264 )
    {
        // we have an h.264 stream
        for (ii = 0; ii < len; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code - remove the ref_idc from the nal type
                uint8_t nal_type = strid & 0x1f;
                if ( nal_type == 0x01 )
                {
                    // Found slice and no recovery point
                    return 0;
                }
                if ( nal_type == 0x05 )
                {
                    // h.264 IDR picture start
                    return 1;
                }
                else if ( nal_type == 0x06 )
                {
                    int off = ii + 1;
                    int recovery_frames = isRecoveryPoint( buf+off, len-off );
                    if ( recovery_frames )
                    {
                        return recovery_frames;
                    }
                }
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( pes->stream_type == 0xea || pes->codec_param == CODEC_ID_VC1 )
    {
        // we have an vc1 stream
        for (ii = 0; ii < len; ii++)
        {
            strid = (strid << 8) | buf[ii];
            if ( strid == 0x10f )
            {
                // the ffmpeg vc1 decoder requires a seq hdr code in the first
                // frame.
                return 1;
            }
        }
        // didn't find an I-frame
        return 0;
    }

    // we don't understand the stream type so just say "yes" otherwise
    // we'll discard all the video.
    return 1;
}

static int ts_isIframe( hb_stream_t *stream, const uint8_t *buf, int adapt_len )
{
    return isIframe( stream, buf + 13 + adapt_len, 188 - ( 13 + adapt_len ) );
}

/*
 * scan the next MB of 'stream' to find the next start packet for
 * the Packetized Elementary Stream associated with TS PID 'pid'.
 */
static const uint8_t *hb_ts_stream_getPEStype(hb_stream_t *stream, uint32_t pid, int *out_adapt_len)
{
    int npack = 300000; // max packets to read

    while (--npack >= 0)
    {
        const uint8_t *buf = next_packet( stream );
        if ( buf == NULL )
        {
            hb_log("hb_ts_stream_getPEStype: EOF while searching for PID 0x%x", pid);
            return 0;
        }

        // while we're reading the stream, check if it has valid PCRs
        // and/or random access points.
        uint32_t pack_pid = ( (buf[1] & 0x1f) << 8 ) | buf[2];
        if ( pack_pid == stream->pmt_info.PCR_PID )
        {
            if ( ( buf[5] & 0x10 ) &&
                 ( ( ( buf[3] & 0x30 ) == 0x20 ) ||
                   ( ( buf[3] & 0x30 ) == 0x30 && buf[4] > 6 ) ) )
            {
                stream->ts_flags |= TS_HAS_PCR;
            }
        }
        if ( buf[5] & 0x40 )
        {
            stream->ts_flags |= TS_HAS_RAP;
        }

        /*
         * The PES header is only in TS packets with 'start' set so we check
         * that first then check for the right PID.
         */
        if ((buf[1] & 0x40) == 0 || pack_pid != pid )
        {
            // not a start packet or not the pid we want
            continue;
        }

        int adapt_len = 0;
        /* skip over the TS hdr to return a pointer to the PES hdr */
        switch (buf[3] & 0x30)
        {
            case 0x00: // illegal
            case 0x20: // fill packet
                continue;

            case 0x30: // adaptation
                adapt_len = buf[4] + 1;
                if (adapt_len > 184)
                {
                    hb_log("hb_ts_stream_getPEStype: invalid adaptation field length %d for PID 0x%x", buf[4], pid);
                    continue;
                }
                break;
        }
        /* PES hdr has to begin with an mpeg start code */
        if (buf[adapt_len+4] == 0x00 && buf[adapt_len+5] == 0x00 && buf[adapt_len+6] == 0x01)
        {
            *out_adapt_len = adapt_len;
            return buf;
        }
    }

    /* didn't find it */
    return 0;
}

static hb_buffer_t * hb_ps_stream_getVideo(
    hb_stream_t *stream,
    hb_pes_info_t *pi)
{
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
    hb_pes_info_t pes_info;
    // how many blocks we read while searching for a video PES header
    int blksleft = 2048;

    while (--blksleft >= 0)
    {
        buf->size = 0;
        int len = hb_ps_read_packet( stream, buf );
        if ( len == 0 )
        {
            // EOF
            break;
        }
        if ( !hb_parse_ps( stream, buf->data, buf->size, &pes_info ) )
            continue;

        int idx;
        if ( pes_info.stream_id == 0xbd )
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.bd_substream_id );
        }
        else
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.stream_id_ext );
        }
        if ( stream->pes.list[idx].stream_kind == V )
        {
            if ( pes_info.pts != -1 )
            {
                *pi = pes_info;
                return buf;
            }
        }
    }
    hb_buffer_close( &buf );
    return NULL;
}

/***********************************************************************
 * hb_stream_duration
 ***********************************************************************
 *
 * Finding stream duration is difficult.  One issue is that the video file
 * may have chunks from several different program fragments (main feature,
 * commercials, station id, trailers, etc.) all with their own base pts
 * value.  We can't find the piece boundaries without reading the entire
 * file but if we compute a rate based on time stamps from two different
 * pieces the result will be meaningless.  The second issue is that the
 * data rate of compressed video normally varies by 5-10x over the length
 * of the video. This says that we want to compute the rate over relatively
 * long segments to get a representative average but long segments increase
 * the likelihood that we'll cross a piece boundary.
 *
 * What we do is take time stamp samples at several places in the file
 * (currently 16) then compute the average rate (i.e., ticks of video per
 * byte of the file) for all pairs of samples (N^2 rates computed for N
 * samples). Some of those rates will be absurd because the samples came
 * from different segments. Some will be way low or high because the
 * samples came from a low or high motion part of the segment. But given
 * that we're comparing *all* pairs the majority of the computed rates
 * should be near the overall average.  So we median filter the computed
 * rates to pick the most representative value.
 *
 **********************************************************************/
struct pts_pos {
    uint64_t pos;   /* file position of this PTS sample */
    uint64_t pts;   /* PTS from video stream */
};

#define NDURSAMPLES 128

// get one (position, timestamp) sampple from a transport or program
// stream.
static struct pts_pos hb_sample_pts(hb_stream_t *stream, uint64_t fpos)
{
    struct pts_pos pp = { 0, 0 };

    if ( stream->hb_stream_type == transport )
    {
        const uint8_t *buf;
        int adapt_len;
        fseeko( stream->file_handle, fpos, SEEK_SET );
        align_to_next_packet( stream );
        int pid = stream->ts.list[ts_index_of_video(stream)].pid;
        buf = hb_ts_stream_getPEStype( stream, pid, &adapt_len );
        if ( buf == NULL )
        {
            hb_log("hb_sample_pts: couldn't find video packet near %"PRIu64, fpos);
            return pp;
        }
        const uint8_t *pes = buf + 4 + adapt_len;
        if ( ( pes[7] >> 7 ) != 1 )
        {
            hb_log("hb_sample_pts: no PTS in video packet near %"PRIu64, fpos);
            return pp;
        }
        pp.pts = ( ( (uint64_t)pes[9] >> 1 ) & 7 << 30 ) |
                 ( (uint64_t)pes[10] << 22 ) |
                 ( ( (uint64_t)pes[11] >> 1 ) << 15 ) |
                 ( (uint64_t)pes[12] << 7 ) |
                 ( (uint64_t)pes[13] >> 1 );

        if ( ts_isIframe( stream, buf, adapt_len ) )
        {
            if (  stream->has_IDRs < 255 )
            {
                ++stream->has_IDRs;
            }
        }
        pp.pos = ftello(stream->file_handle);
        if ( !stream->has_IDRs )
        {
            // Scan a little more to see if we will stumble upon one
            int ii;
            for ( ii = 0; ii < 10; ii++ )
            {
                buf = hb_ts_stream_getPEStype( stream, pid, &adapt_len );
                if ( buf == NULL )
                    break;
                if ( ts_isIframe( stream, buf, adapt_len ) )
                {
                    ++stream->has_IDRs;
                    break;
                }
            }
        }
    }
    else
    {
        hb_buffer_t *buf;
        hb_pes_info_t pes_info;

        // round address down to nearest dvd sector start
        fpos &=~ ( HB_DVD_READ_BUFFER_SIZE - 1 );
        fseeko( stream->file_handle, fpos, SEEK_SET );
        if ( stream->hb_stream_type == program )
        {
            skip_to_next_pack( stream );
        }
        buf = hb_ps_stream_getVideo( stream, &pes_info );
        if ( buf == NULL )
        {
            hb_log("hb_sample_pts: couldn't find video packet near %"PRIu64, fpos);
            return pp;
        }
        if ( pes_info.pts < 0 )
        {
            hb_log("hb_sample_pts: no PTS in video packet near %"PRIu64, fpos);
            hb_buffer_close( &buf );
            return pp;
        }
        if ( isIframe( stream, buf->data, buf->size ) )
        {
            if (  stream->has_IDRs < 255 )
            {
                ++stream->has_IDRs;
            }
        }
        hb_buffer_close( &buf );
        if ( !stream->has_IDRs )
        {
            // Scan a little more to see if we will stumble upon one
            int ii;
            for ( ii = 0; ii < 10; ii++ )
            {
                buf = hb_ps_stream_getVideo( stream, &pes_info );
                if ( buf == NULL )
                    break;
                if ( isIframe( stream, buf->data, buf->size ) )
                {
                    ++stream->has_IDRs;
                    hb_buffer_close( &buf );
                    break;
                }
                hb_buffer_close( &buf );
            }
        }

        pp.pts = pes_info.pts;
        pp.pos = ftello(stream->file_handle);
    }
    return pp;
}

static int dur_compare( const void *a, const void *b )
{
    const double *aval = a, *bval = b;
    return ( *aval < *bval ? -1 : ( *aval == *bval ? 0 : 1 ) );
}

// given an array of (position, time) samples, compute a max-likelihood
// estimate of the average rate by computing the rate between all pairs
// of samples then taking the median of those rates.
static double compute_stream_rate( struct pts_pos *pp, int n )
{
    int i, j;
    double rates[NDURSAMPLES * NDURSAMPLES / 8];
    double *rp = rates;

    // the following nested loops compute the rates between all pairs.
    *rp = 0;
    for ( i = 0; i < n-1; ++i )
    {
        // Bias the median filter by not including pairs that are "far"
        // from one another. This is to handle cases where the file is
        // made of roughly equal size pieces where a symmetric choice of
        // pairs results in having the same number of intra-piece &
        // inter-piece rate estimates. This would mean that the median
        // could easily fall in the inter-piece part of the data which
        // would give a bogus estimate. The 'ns' index creates an
        // asymmetry that favors locality.
        int ns = i + ( n >> 3 );
        if ( ns > n )
            ns = n;
        for ( j = i+1; j < ns; ++j )
        {
            if ( (uint64_t)(pp[j].pts - pp[i].pts) > 90000LL*3600*6 )
                break;
            if ( pp[j].pts != pp[i].pts && pp[j].pos > pp[i].pos )
            {
                *rp = ((double)( pp[j].pts - pp[i].pts )) /
                      ((double)( pp[j].pos - pp[i].pos ));
                ++rp;
            }
        }
    }
    // now compute and return the median of all the (n*n/2) rates we computed
    // above.
    int nrates = rp - rates;
    qsort( rates, nrates, sizeof (rates[0] ), dur_compare );
    return rates[nrates >> 1];
}

static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle)
{
    struct pts_pos ptspos[NDURSAMPLES];
    struct pts_pos *pp = ptspos;
    int i;

    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    uint64_t fincr = fsize / NDURSAMPLES;
    uint64_t fpos = fincr / 2;
    for ( i = NDURSAMPLES; --i >= 0; fpos += fincr )
    {
        *pp++ = hb_sample_pts(stream, fpos);
    }
    uint64_t dur = compute_stream_rate( ptspos, pp - ptspos ) * (double)fsize;
    inTitle->duration = dur;
    dur /= 90000;
    inTitle->hours    = dur / 3600;
    inTitle->minutes  = ( dur % 3600 ) / 60;
    inTitle->seconds  = dur % 60;

    rewind(stream->file_handle);
}

/***********************************************************************
 * hb_stream_read
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_stream_read( hb_stream_t * src_stream )
{
    if ( src_stream->hb_stream_type == ffmpeg )
    {
        return hb_ffmpeg_read( src_stream );
    }
    if ( src_stream->hb_stream_type == program )
    {
        return hb_ps_stream_decode( src_stream );
    }
    return hb_ts_stream_decode( src_stream );
}

int64_t ffmpeg_initial_timestamp( hb_stream_t * stream )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    if ( ic->start_time != AV_NOPTS_VALUE && ic->start_time > 0 )
        return ic->start_time;
    else
        return 0;
}

int hb_stream_seek_chapter( hb_stream_t * stream, int chapter_num )
{

    if ( stream->hb_stream_type != ffmpeg )
    {
        // currently meaningliess for transport and program streams
        return 1;
    }
    if ( !stream || !stream->title ||
         chapter_num > hb_list_count( stream->title->list_chapter ) )
    {
        return 0;
    }

    int64_t sum_dur = 0;
    hb_chapter_t *chapter = NULL;
    int i;
    for ( i = 0; i < chapter_num; ++i)
    {
        chapter = hb_list_item( stream->title->list_chapter, i );
        sum_dur += chapter->duration;
    }
    stream->chapter = chapter_num - 1;
    stream->chapter_end = sum_dur;

    int64_t pos = ( ( ( sum_dur - chapter->duration ) * AV_TIME_BASE ) / 90000 ) + ffmpeg_initial_timestamp( stream );

    hb_deep_log( 2, "Seeking to chapter %d: starts %"PRId64", ends %"PRId64", AV pos %"PRId64,
                 chapter_num, sum_dur - chapter->duration, sum_dur, pos);

    if ( chapter_num > 1 && pos > 0 )
    {
        AVStream *st = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id];
        // timebase must be adjusted to match timebase of stream we are
        // using for seeking.
        pos = av_rescale(pos, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num);
        avformat_seek_file( stream->ffmpeg_ic, stream->ffmpeg_video_id, 0, pos, pos, AVSEEK_FLAG_BACKWARD);
    }
    return 1;
}

/***********************************************************************
 * hb_stream_chapter
 ***********************************************************************
 * Return the number of the chapter that we are currently in. We store
 * the chapter number starting from 0, so + 1 for the real chpater num.
 **********************************************************************/
int hb_stream_chapter( hb_stream_t * src_stream )
{
    return( src_stream->chapter + 1 );
}

/***********************************************************************
 * hb_stream_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_stream_seek( hb_stream_t * stream, float f )
{
    if ( stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek( stream, f );
    }
    off_t stream_size, cur_pos, new_pos;
    double pos_ratio = f;
    cur_pos = ftello( stream->file_handle );
    fseeko( stream->file_handle, 0, SEEK_END );
    stream_size = ftello( stream->file_handle );
    new_pos = (off_t) ((double) (stream_size) * pos_ratio);
    new_pos &=~ (HB_DVD_READ_BUFFER_SIZE - 1);

    int r = fseeko( stream->file_handle, new_pos, SEEK_SET );
    if (r == -1)
    {
        fseeko( stream->file_handle, cur_pos, SEEK_SET );
        return 0;
    }

    if ( stream->hb_stream_type == transport )
    {
        // We need to drop the current decoder output and move
        // forwards to the next transport stream packet.
        hb_ts_stream_reset(stream);
        align_to_next_packet(stream);
        if ( !stream->has_IDRs )
        {
            // the stream has no IDRs so don't look for one.
            stream->need_keyframe = 0;
        }
    }
    else if ( stream->hb_stream_type == program )
    {
        hb_ps_stream_reset(stream);
        skip_to_next_pack( stream );
        if ( !stream->has_IDRs )
        {
            // the stream has no IDRs so don't look for one.
            stream->need_keyframe = 0;
        }
    }

    return 1;
}

int hb_stream_seek_ts( hb_stream_t * stream, int64_t ts )
{
    if ( stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek_ts( stream, ts );
    }
    return -1;
}

static char* strncpyupper( char *dst, const char *src, int len )
{
    int ii;

    for ( ii = 0; ii < len-1 && src[ii]; ii++ )
    {
        dst[ii] = islower(src[ii]) ? toupper(src[ii]) : src[ii];
    }
    dst[ii] = '\0';
    return dst;
}

static const char *stream_type_name2(hb_stream_t *stream, hb_pes_stream_t *pes)
{
    static char codec_name_caps[80];

    if ( stream->reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // Names for streams we know about.
        switch ( pes->stream_type )
        {
            case 0x80:
                return "BD LPCM";

            case 0x83:
                return "TrueHD";

            case 0x84:
                return "E-AC3";

            case 0x85:
                return "DTS-HD HRA";

            case 0x86:
                return "DTS-HD MA";

            default:
                break;
        }
    }
    if ( st2codec[pes->stream_type].name )
    {
        return st2codec[pes->stream_type].name;
    }
    if ( pes->codec_name[0] != 0 )
    {
        return pes->codec_name;
    }
    if ( pes->codec & HB_ACODEC_FF_MASK )
    {
        AVCodec * codec = avcodec_find_decoder( pes->codec_param );
        if ( codec && codec->name && codec->name[0] )
        {
            strncpyupper( codec_name_caps, codec->name, 80 );
            return codec_name_caps;
        }
    }
    return "Unknown";
}

static const char *stream_type_name (uint32_t reg_desc, uint8_t stream_type)
{
    if ( reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // Names for streams we know about.
        switch ( stream_type )
        {
            case 0x80:
                return "BD LPCM";

            case 0x83:
                return "TrueHD";

            case 0x84:
                return "E-AC3";

            case 0x85:
                return "DTS-HD HRA";

            case 0x86:
                return "DTS-HD MA";

            default:
                break;
        }
    }
    return st2codec[stream_type].name ? st2codec[stream_type].name : "Unknown";
}

static void set_audio_description(
    hb_stream_t * stream,
    hb_audio_t *audio,
    iso639_lang_t *lang)
{
    int idx;
    const char *codec_name;
    char codec_name_caps[80];
    AVCodecContext *cc = NULL;

    if ( stream && stream->ffmpeg_ic )
    {
        cc = stream->ffmpeg_ic->streams[audio->id]->codec;
    }

    // Names for streams we know about.
    if ( audio->config.in.stream_type == 0x80 &&
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // LPCM audio in bluray have an stype of 0x80
        codec_name = "BD LPCM";
    }
    else if ( audio->config.in.stream_type == 0x83 &&
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // This is an interleaved TrueHD/AC-3 stream and the esid of
        // the AC-3 is 0x76
        if (audio->config.in.substream_type == HB_SUBSTREAM_BD_AC3)
            codec_name = "AC3";
        else
            codec_name = "TrueHD";
    }
    else if ( audio->config.in.stream_type == 0x86 &&
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // This is an interleaved DTS-HD MA/DTS stream and the
        // esid of the DTS is 0x71
        if (audio->config.in.substream_type == HB_SUBSTREAM_BD_DTS)
            codec_name = "DTS";
        else
            codec_name = "DTS-HD MA";
    }
    else if ( audio->config.in.stream_type == 0x85 &&
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // DTS-HD HRA audio in bluray has an stype of 0x85
        // which conflicts with ATSC Program ID
        // To distinguish, Bluray streams have a reg_desc of HDMV
        // This is an interleaved DTS-HD HRA/DTS stream and the
        // esid of the DTS is 0x71
        if (audio->config.in.substream_type == HB_SUBSTREAM_BD_DTS)
            codec_name = "DTS";
        else
            codec_name = "DTS-HD HRA";
    }
    else if ( audio->config.in.stream_type == 0x84 &&
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // EAC3 audio in bluray has an stype of 0x84
        // which conflicts with SDDS
        // To distinguish, Bluray streams have a reg_desc of HDMV
        codec_name = "E-AC3";
    }
    // For streams demuxed and decoded by ffmpeg, we have a cached context.
    // Use it to get the name and profile information.  Obtaining
    // the profile requires that ffmpeg has already probed the stream.
    else if ( ( audio->config.in.codec & HB_ACODEC_FF_MASK ) && cc &&
         avcodec_find_decoder( cc->codec_id ) )
    {
        AVCodec *codec = avcodec_find_decoder( cc->codec_id );
        codec_name = codec->name;

        const char *profile_name;
        profile_name = av_get_profile_name( codec, cc->profile );
        if ( profile_name )
        {
            codec_name = profile_name;
        }
    }
    else if ( stream->hb_stream_type != ffmpeg && 
              (idx = index_of_id( stream, audio->id ) ) >= 0 )
    {
        codec_name = stream_type_name2( stream, &stream->pes.list[idx] );
    }
    else if ( st2codec[audio->config.in.stream_type].kind == A )
    {
        codec_name = stream_type_name(audio->config.in.reg_desc,
                                      audio->config.in.stream_type);
    }
    // For streams demuxed by us and decoded by ffmpeg, we can lookup the
    // decoder name.
    else if ( ( audio->config.in.codec & HB_ACODEC_FF_MASK ) &&
              avcodec_find_decoder( audio->config.in.codec_param ) )
    {
        codec_name = avcodec_find_decoder( audio->config.in.codec_param )->name;
        strncpyupper( codec_name_caps, codec_name, 80 );
        codec_name = codec_name_caps;
    }
    else
    {
        codec_name = audio->config.in.codec == HB_ACODEC_AC3 ? "AC3" :
                 audio->config.in.codec == HB_ACODEC_DCA ? "DTS" :
                 audio->config.in.codec == HB_ACODEC_DCA_HD ? "DTS-HD" :
                 audio->config.in.codec == HB_ACODEC_LPCM ? "LPCM" :
                 (audio->config.in.codec & HB_ACODEC_FF_MASK) ? "FFmpeg" :
                 "Unknown";
    }

    snprintf( audio->config.lang.description,
              sizeof( audio->config.lang.description ), "%s (%s)",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name,
              codec_name );

    if ( audio->config.in.channel_layout )
    {
        int layout = audio->config.in.channel_layout;
        char *desc = audio->config.lang.description +
                        strlen( audio->config.lang.description );
        sprintf( desc, " (%d.%d ch)",
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(layout) +
                     HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(layout),
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(layout) );
    }

    snprintf( audio->config.lang.simple, sizeof( audio->config.lang.simple ), "%s",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name );
    snprintf( audio->config.lang.iso639_2, sizeof( audio->config.lang.iso639_2 ),
              "%s", lang->iso639_2);
}

// Sort specifies the index in the audio list where you would
// like sorted items to begin.
static void pes_add_audio_to_title(
    hb_stream_t *stream,
    int         idx,
    hb_title_t  *title,
    int         sort)
{
    hb_pes_stream_t *pes = &stream->pes.list[idx];

    // Sort by id when adding to the list
    // This assures that they are always displayed in the same order
    int id = get_id( pes );
    int i;
    hb_audio_t *tmp = NULL;

    int count = hb_list_count( title->list_audio );

    // Don't add the same audio twice.  Search for audio.
    for ( i = 0; i < count; i++ )
    {
        tmp = hb_list_item( title->list_audio, i );
        if ( id == tmp->id )
            return;
    }

    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = id;
    audio->config.in.reg_desc = stream->reg_desc;
    audio->config.in.stream_type = pes->stream_type;
    audio->config.in.substream_type = pes->stream_id_ext;

    audio->config.in.codec = pes->codec;
    audio->config.in.codec_param = pes->codec_param;

    set_audio_description( stream, audio, lang_for_code( pes->lang_code ) );

    hb_log("stream id 0x%x (type 0x%x substream 0x%x) audio 0x%x",
           pes->stream_id, pes->stream_type, pes->stream_id_ext, audio->id);

    audio->config.in.track = idx;

    // Search for the sort position
    if ( sort >= 0 )
    {
        sort = sort < count ? sort : count;
        for ( i = sort; i < count; i++ )
        {
            tmp = hb_list_item( title->list_audio, i );
            int sid = tmp->id & 0xffff;
            int ssid = tmp->id >> 16;
            if ( pes->stream_id < sid )
                break;
            else if ( pes->stream_id <= sid &&
                      pes->stream_id_ext <= ssid )
            {
                break;
            }
        }
        hb_list_insert( title->list_audio, i, audio );
    }
    else
    {
        hb_list_add( title->list_audio, audio );
    }
}

static void hb_init_audio_list(hb_stream_t *stream, hb_title_t *title)
{
    int ii;
    int map_idx;
    int largest = -1;

    // First add all that were found in a map.
    for ( map_idx = 0; 1; map_idx++ )
    {
        for ( ii = 0; ii < stream->pes.count; ii++ )
        {
            if ( stream->pes.list[ii].stream_kind == A )
            {
                if ( stream->pes.list[ii].map_idx == map_idx )
                {
                    pes_add_audio_to_title( stream, ii, title, -1 );
                }
                if ( stream->pes.list[ii].map_idx > largest )
                    largest = stream->pes.list[ii].map_idx;
            }
        }
        if ( map_idx > largest )
            break;
    }

    int count = hb_list_count( title->list_audio );
    // Now add the reset.  Sort them by stream id.
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == A )
        {
            pes_add_audio_to_title( stream, ii, title, count );
        }
    }
}

/***********************************************************************
 * hb_ts_stream_init
 ***********************************************************************
 *
 **********************************************************************/

static void hb_ts_stream_init(hb_stream_t *stream)
{
    int i;

    if ( stream->ts.list )
    {
        for (i=0; i < stream->ts.alloc; i++)
        {
            stream->ts.list[i].continuity = -1;
            stream->ts.list[i].pid = -1;
            stream->ts.list[i].pes_list = -1;
        }
    }
    stream->ts.count = 0;

    if ( stream->pes.list )
    {
        for (i=0; i < stream->pes.alloc; i++)
        {
            stream->pes.list[i].stream_id = -1;
            stream->pes.list[i].next = -1;
        }
    }
    stream->pes.count = 0;

    stream->ts.packet = malloc( stream->packetsize );

    // Find the audio and video pids in the stream
    hb_ts_stream_find_pids(stream);

    // hb_ts_resolve_pid_types reads some data, so the TS buffers
    // are needed here.
    for (i = 0; i < stream->ts.count; i++)
    {
        // demuxing buffer for TS to PS conversion
        stream->ts.list[i].buf = hb_buffer_init(stream->packetsize);
        stream->ts.list[i].extra_buf = hb_buffer_init(stream->packetsize);
        stream->ts.list[i].buf->size = 0;
        stream->ts.list[i].extra_buf->size = 0;
    }
    hb_ts_resolve_pid_types(stream);

    if( stream->scan )
    {
        hb_log("Found the following PIDS");
        hb_log("    Video PIDS : ");
        for (i=0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == V )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream, 
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
        }
        hb_log("    Audio PIDS : ");
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == A )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream,
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
        }
        hb_log("    Other PIDS : ");
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == N ||
                 ts_stream_kind( stream, i ) == P )
            {
                hb_log( "      0x%x type %s (0x%x)%s",
                        stream->ts.list[i].pid,
                        stream_type_name2(stream,
                                &stream->pes.list[stream->ts.list[i].pes_list]),
                        ts_stream_type( stream, i ),
                        stream->ts.list[i].is_pcr ? " (PCR)" : "");
            }
            if ( ts_stream_kind( stream, i ) == N )
                hb_stream_delete_ts_entry(stream, i);
        }
    }
    else
    {
        for (i = 0; i < stream->ts.count; i++)
        {
            if ( ts_stream_kind( stream, i ) == N )
                hb_stream_delete_ts_entry(stream, i);
        }
    }
}

static void hb_ps_stream_init(hb_stream_t *stream)
{
    int i;

    if ( stream->pes.list )
    {
        for (i=0; i < stream->pes.alloc; i++)
        {
            stream->pes.list[i].stream_id = -1;
            stream->pes.list[i].next = -1;
        }
    }
    stream->pes.count = 0;

    // Find the audio and video pids in the stream
    hb_ps_stream_find_streams(stream);
    hb_ps_resolve_stream_types(stream);

    if( stream->scan )
    {
        hb_log("Found the following streams");
        hb_log("    Video Streams : ");
        for (i=0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == V )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type);
            }
        }
        hb_log("    Audio Streams : ");
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == A )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type );
            }
        }
        hb_log("    Other Streams : ");
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == N )
            {
                hb_log( "      0x%x-0x%x type %s (0x%x)",
                        stream->pes.list[i].stream_id,
                        stream->pes.list[i].stream_id_ext,
                        stream_type_name2(stream,
                                         &stream->pes.list[i]),
                        stream->pes.list[i].stream_type );
                hb_stream_delete_ps_entry(stream, i);
            }
        }
    }
    else
    {
        for (i = 0; i < stream->pes.count; i++)
        {
            if ( stream->pes.list[i].stream_kind == N )
                hb_stream_delete_ps_entry(stream, i);
        }
    }
}

#define MAX_HOLE 208*80

static off_t align_to_next_packet(hb_stream_t *stream)
{
    uint8_t buf[MAX_HOLE];
    off_t pos = 0;
    off_t start = ftello(stream->file_handle);
    off_t orig;

    if ( start >= stream->packetsize ) {
        start -= stream->packetsize;
        fseeko(stream->file_handle, start, SEEK_SET);
    }
    orig = start;

    while (1)
    {
        if (fread(buf, sizeof(buf), 1, stream->file_handle) == 1)
        {
            const uint8_t *bp = buf;
            int i;

            for ( i = sizeof(buf) - 8 * stream->packetsize; --i >= 0; ++bp )
            {
                if ( have_ts_sync( bp, stream->packetsize, 8 ) )
                {
                    break;
                }
            }
            if ( i >= 0 )
            {
                pos = ( bp - buf ) - stream->packetsize + 188;
                break;
            }
            fseeko(stream->file_handle, -8 * stream->packetsize, SEEK_CUR);
            start = ftello(stream->file_handle);
        }
        else
        {
            return 0;
        }
    }
    fseeko(stream->file_handle, start+pos, SEEK_SET);
    return start - orig + pos;
}

static const unsigned int bitmask[] = {
    0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,
    0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff,
    0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff,
    0x1ffffff,0x3ffffff,0x7ffffff,0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};

static inline void bits_init(bitbuf_t *bb, uint8_t* buf, int bufsize, int clear)
{
    bb->pos = 0;
    bb->buf = buf;
    bb->size = bufsize;
    bb->val = (bb->buf[0] << 24) | (bb->buf[1] << 16) |
              (bb->buf[2] << 8) | bb->buf[3];
    if (clear)
        memset(bb->buf, 0, bufsize);
    bb->size = bufsize;
}

static inline void bits_clone( bitbuf_t *dst, bitbuf_t *src, int bufsize )
{
    *dst = *src;
    dst->size = (dst->pos >> 3) + bufsize;
}

static inline int bits_bytes_left(bitbuf_t *bb)
{
    return bb->size - (bb->pos >> 3);
}

static inline int bits_eob(bitbuf_t *bb)
{
    return bb->pos >> 3 == bb->size;
}

static inline unsigned int bits_peek(bitbuf_t *bb, int bits)
{
    unsigned int val;
    int left = 32 - (bb->pos & 31);

    if (bits < left)
    {
        val = (bb->val >> (left - bits)) & bitmask[bits];
    }
    else
    {
        val = (bb->val & bitmask[left]) << (bits - left);
        int bpos = bb->pos + left;
        bits -= left;


        if (bits > 0)
        {
            int pos = bpos >> 3;
            int bval =  (bb->buf[pos]     << 24) | 
                        (bb->buf[pos + 1] << 16) | 
                        (bb->buf[pos + 2] <<  8) | 
                         bb->buf[pos + 3];
            val |= (bval >> (32 - bits)) & bitmask[bits];
        }
    }

    return val;
}

static inline unsigned int bits_get(bitbuf_t *bb, int bits)
{
    unsigned int val;
    int left = 32 - (bb->pos & 31);

    if (bits < left)
    {
        val = (bb->val >> (left - bits)) & bitmask[bits];
        bb->pos += bits;
    }
    else
    {
        val = (bb->val & bitmask[left]) << (bits - left);
        bb->pos += left;
        bits -= left;

        int pos = bb->pos >> 3;
        bb->val = (bb->buf[pos] << 24) | (bb->buf[pos + 1] << 16) | (bb->buf[pos + 2] << 8) | bb->buf[pos + 3];

        if (bits > 0)
        {
            val |= (bb->val >> (32 - bits)) & bitmask[bits];
            bb->pos += bits;
        }
    }

    return val;
}

static inline int bits_read_ue(bitbuf_t *bb )
{
    int ii = 0;

    while( bits_get( bb, 1 ) == 0 && !bits_eob( bb ) && ii < 32 )
    {
        ii++;
    }
    return( ( 1 << ii) - 1 + bits_get( bb, ii ) );
}

static inline int bits_skip(bitbuf_t *bb, int bits)
{
    if (bits <= 0)
        return 0;
    while (bits > 32)
    {
        bits_get(bb, 32);
        bits -= 32;
    }
    bits_get(bb, bits);
    return 0;
}

// extract what useful information we can from the elementary stream
// descriptor list at 'dp' and add it to the stream at 'esindx'.
// Descriptors with info we don't currently use are ignored.
// The descriptor list & descriptor item formats are defined in
// ISO 13818-1 (2000E) section 2.6 (pg. 62).
static void decode_element_descriptors(
    hb_stream_t   *stream,
    int           pes_idx,
    bitbuf_t   *bb)
{
    int ii;

    while( bits_bytes_left( bb ) > 2 )
    {
        uint8_t tag = bits_get(bb, 8);
        uint8_t len = bits_get(bb, 8);
        switch ( tag )
        {
            case 5:    // Registration descriptor
                stream->pes.list[pes_idx].format_id = bits_get(bb, 32);
                bits_skip(bb, 8 * (len - 4));
                break;

            case 10:    // ISO_639_language descriptor
            {
                char code[3];
                for (ii = 0; ii < 3; ii++)
                {
                    code[ii] = bits_get(bb, 8);
                }
                stream->pes.list[pes_idx].lang_code =
                                    lang_to_code(lang_for_code2(code));
                bits_skip(bb, 8 * (len - 3));
            } break;

            case 0x56:  // DVB Teletext descriptor
            {
                // We don't currently process teletext from
                // TS or PS streams.  Set stream 'kind' to N
                stream->pes.list[pes_idx].stream_type = 0x00;
                stream->pes.list[pes_idx].stream_kind = N;
                strncpy(stream->pes.list[pes_idx].codec_name,
                        "DVB Teletext", 80);
                bits_skip(bb, 8 * len);
            } break;

            case 0x59:  // DVB Subtitleing descriptor
            {
                // We don't currently process subtitles from
                // TS or PS streams.  Set stream 'kind' to N
                stream->pes.list[pes_idx].stream_type = 0x00;
                stream->pes.list[pes_idx].stream_kind = N;
                strncpy(stream->pes.list[pes_idx].codec_name,
                        "DVB Subtitling", 80);
                bits_skip(bb, 8 * len);
            } break;

            case 0x6a:  // DVB AC-3 descriptor
            {
                stream->pes.list[pes_idx].stream_type = 0x81;
                update_pes_kind( stream, pes_idx );
                bits_skip(bb, 8 * len);
            } break;

            case 0x7a:  // DVB EAC-3 descriptor
            {
                stream->pes.list[pes_idx].stream_type = 0x87;
                update_pes_kind( stream, pes_idx );
                bits_skip(bb, 8 * len);
            } break;

            default:
                bits_skip(bb, 8 * len);
                break;
        }
    }
}

int decode_program_map(hb_stream_t* stream)
{
    bitbuf_t bb;
    bits_init(&bb, stream->pmt_info.tablebuf, stream->pmt_info.tablepos, 0);

    bits_get(&bb, 8);  // table_id
    bits_get(&bb, 4);
    unsigned int section_length = bits_get(&bb, 12);

    bits_get(&bb, 16); // program number
    bits_get(&bb, 2);
    bits_get(&bb, 5);  // version_number
    bits_get(&bb, 1);
    bits_get(&bb, 8);  // section_number
    bits_get(&bb, 8);  // last_section_number
    bits_get(&bb, 3);
    stream->pmt_info.PCR_PID = bits_get(&bb, 13);
    bits_get(&bb, 4);
    int program_info_length = bits_get(&bb, 12);

    int i;
    for (i = 0; i < program_info_length - 2; )
    {
        uint8_t tag, len;
        tag = bits_get(&bb, 8);
        len = bits_get(&bb, 8);
        i += 2;
        if ( i + len > program_info_length )
        {
            break;
        }
        if (tag == 0x05 && len >= 4)
        {
            // registration descriptor
            stream->reg_desc = bits_get(&bb, 32);
            i += 4;
            len -= 4;
        }
        int j;
        for ( j = 0; j < len; j++ )
        {
            bits_get(&bb, 8);
        }
        i += len;
    }
    for ( ; i < program_info_length; i++ )
    {
        bits_get(&bb, 8);
    }

    int cur_pos =  9 /* data after the section length field*/ + program_info_length;
    int done_reading_stream_types = 0;
    int ii = 0;
    while (!done_reading_stream_types)
    {
        unsigned char stream_type = bits_get(&bb, 8);
        bits_get(&bb, 3);
        unsigned int elementary_PID = bits_get(&bb, 13);
        bits_get(&bb, 4);
        unsigned int info_len = bits_get(&bb, 12);
        // Defined audio stream types are 0x81 for AC-3/A52 audio
        // and 0x03 for mpeg audio. But content producers seem to
        // use other values (0x04 and 0x06 have both been observed)
        // so at this point we say everything that isn't a video
        // pid is audio then at the end of hb_stream_title_scan
        // we'll figure out which are really audio by looking at
        // the PES headers.
        int pes_idx;
        update_ts_streams( stream, elementary_PID, 0,
                           stream_type, -1, &pes_idx );
        if ( pes_idx >= 0 )
            stream->pes.list[pes_idx].map_idx = ii;
        if (info_len > 0)
        {
            bitbuf_t bb_desc;
            bits_clone( &bb_desc, &bb, info_len );
            if ( pes_idx >= 0 )
                decode_element_descriptors( stream, pes_idx, &bb_desc );
            bits_skip(&bb, 8 * info_len);
        }

        cur_pos += 5 /* stream header */ + info_len;

        if (cur_pos >= section_length - 4 /* stop before the CRC */)
            done_reading_stream_types = 1;
        ii++;
    }

    return 1;
}

static int build_program_map(const uint8_t *buf, hb_stream_t *stream)
{
    // Get adaption header info
    int adapt_len = 0;
    int adaption = (buf[3] & 0x30) >> 4;
    if (adaption == 0)
            return 0;
    else if (adaption == 0x2)
            adapt_len = 184;
    else if (adaption == 0x3)
            adapt_len = buf[4] + 1;
    if (adapt_len > 184)
            return 0;

    // Get payload start indicator
    int start;
    start = (buf[1] & 0x40) != 0;

    // Get pointer length - only valid in packets with a start flag
    int pointer_len = 0;

    if (start)
    {
        pointer_len = buf[4 + adapt_len] + 1;
        stream->pmt_info.tablepos = 0;
    }
    // Get Continuity Counter
    int continuity_counter = buf[3] & 0x0f;
    if (!start && (stream->pmt_info.current_continuity_counter + 1 != continuity_counter))
    {
        hb_log("build_program_map - Continuity Counter %d out of sequence - expected %d", continuity_counter, stream->pmt_info.current_continuity_counter+1);
        return 0;
    }
    stream->pmt_info.current_continuity_counter = continuity_counter;
    stream->pmt_info.reading |= start;

    // Add the payload for this packet to the current buffer
    int amount_to_copy = 184 - adapt_len - pointer_len;
    if (stream->pmt_info.reading && (amount_to_copy > 0))
    {
            stream->pmt_info.tablebuf = realloc(stream->pmt_info.tablebuf, stream->pmt_info.tablepos + amount_to_copy);

            memcpy(stream->pmt_info.tablebuf + stream->pmt_info.tablepos, buf + 4 + adapt_len + pointer_len, amount_to_copy);
            stream->pmt_info.tablepos += amount_to_copy;
    }
    if (stream->pmt_info.tablepos > 3)
    {
        // We have enough to check the section length
        int length;
        length = ((stream->pmt_info.tablebuf[1] << 8) +
                  stream->pmt_info.tablebuf[2]) & 0xFFF;
        if (stream->pmt_info.tablepos > length + 1)
        {
            // We just finished a bunch of packets - parse the program map details
            int decode_ok = 0;
            if (stream->pmt_info.tablebuf[0] == 0x02)
                decode_ok = decode_program_map(stream);
            free(stream->pmt_info.tablebuf);
            stream->pmt_info.tablebuf = NULL;
            stream->pmt_info.tablepos = 0;
            stream->pmt_info.reading = 0;
            if (decode_ok)
                return decode_ok;
        }

    }

    return 0;
}

static int decode_PAT(const uint8_t *buf, hb_stream_t *stream)
{
    unsigned char tablebuf[1024];
    unsigned int tablepos = 0;

    int reading = 0;


    // Get adaption header info
    int adapt_len = 0;
    int adaption = (buf[3] & 0x30) >> 4;
    if (adaption == 0)
            return 0;
    else if (adaption == 0x2)
            adapt_len = 184;
    else if (adaption == 0x3)
            adapt_len = buf[4] + 1;
    if (adapt_len > 184)
            return 0;

    // Get pointer length
    int pointer_len = buf[4 + adapt_len] + 1;

    // Get payload start indicator
    int start;
    start = (buf[1] & 0x40) != 0;

    if (start)
            reading = 1;

    // Add the payload for this packet to the current buffer
    if (reading && (184 - adapt_len) > 0)
    {
            if (tablepos + 184 - adapt_len - pointer_len > 1024)
            {
                    hb_log("decode_PAT - Bad program section length (> 1024)");
                    return 0;
            }
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + pointer_len, 184 - adapt_len - pointer_len);
            tablepos += 184 - adapt_len - pointer_len;
    }

    if (start && reading)
    {
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + 1, pointer_len - 1);


            unsigned int pos = 0;
            //while (pos < tablepos)
            {
                    bitbuf_t bb;
                    bits_init(&bb, tablebuf + pos, tablepos - pos, 0);

                    unsigned char section_id    = bits_get(&bb, 8);
                    bits_get(&bb, 4);
                    unsigned int section_len    = bits_get(&bb, 12);
                    bits_get(&bb, 16); // transport_id
                    bits_get(&bb, 2);
                    bits_get(&bb, 5);  // version_num
                    bits_get(&bb, 1);  // current_next
                    bits_get(&bb, 8);  // section_num
                    bits_get(&bb, 8);  // last_section

                    switch (section_id)
                    {
                      case 0x00:
                        {
                          // Program Association Section
                          section_len -= 5;    // Already read transport stream ID, version num, section num, and last section num
                          section_len -= 4;   // Ignore the CRC
                          int curr_pos = 0;
                          stream->ts_number_pat_entries = 0;
                          while ((curr_pos < section_len) && (stream->ts_number_pat_entries < kMaxNumberPMTStreams))
                          {
                            unsigned int pkt_program_num = bits_get(&bb, 16);
                            stream->pat_info[stream->ts_number_pat_entries].program_number = pkt_program_num;

                            bits_get(&bb, 3);  // Reserved
                            if (pkt_program_num == 0)
                            {
                              bits_get(&bb, 13); // pkt_network_id
                            }
                            else
                            {
                              unsigned int pkt_program_map_PID = bits_get(&bb, 13);
                                stream->pat_info[stream->ts_number_pat_entries].program_map_PID = pkt_program_map_PID;
                            }
                            curr_pos += 4;
                            stream->ts_number_pat_entries++;
                          }
                        }
                        break;
                      case 0xC7:
                            {
                                    break;
                            }
                      case 0xC8:
                            {
                                    break;
                            }
                    }

                    pos += 3 + section_len;
            }

            tablepos = 0;
    }
    return 1;
}

// convert a PES PTS or DTS to an int64
static int64_t parse_pes_timestamp( bitbuf_t *bb )
{
    int64_t ts;

    ts =  ( (uint64_t)   bits_get(bb,  3) << 30 ) +
                         bits_skip(bb, 1)         +
                       ( bits_get(bb, 15) << 15 ) +
                         bits_skip(bb, 1)         +
                         bits_get(bb, 15);
    bits_skip(bb, 1);
    return ts;
}

static int parse_pes_header(
    hb_stream_t   *stream,
    bitbuf_t      *bb,
    hb_pes_info_t *pes_info )
{
    if ( bits_bytes_left(bb) < 6 )
    {
        return 0;
    }

    bits_skip(bb, 8 * 4);
    pes_info->packet_len = bits_get(bb, 16);

    /*
     * This would normally be an error.  But the decoders can generally
     * recover well from missing data.  So let the packet pass.
    if ( bits_bytes_left(bb) < pes_info->packet_len )
    {
        return 0;
    }
    */

    int mark = bits_peek(bb, 2);
    if ( mark == 0x02 )
    {
        // mpeg2 pes
        if ( bits_bytes_left(bb) < 3 )
        {
            return 0;
        }

        /*
        bits_skip(bb, 2);
        bits_get(bb, 2);    // scrambling
        bits_get(bb, 1);    // priority
        bits_get(bb, 1);    // alignment
        bits_get(bb, 1);    // copyright
        bits_get(bb, 1);    // original
        */
        bits_get(bb, 8);    // skip all of the above

        int has_pts        = bits_get(bb, 2);
        int has_escr       = bits_get(bb, 1);
        int has_esrate     = bits_get(bb, 1);
        int has_dsm        = bits_get(bb, 1);
        int has_copy_info  = bits_get(bb, 1);
        int has_crc        = bits_get(bb, 1);
        int has_ext        = bits_get(bb, 1);
        int hdr_len = pes_info->header_len = bits_get(bb, 8);
        pes_info->header_len += bb->pos >> 3;

        bitbuf_t bb_hdr;
        bits_clone(&bb_hdr, bb, hdr_len);

        if ( bits_bytes_left(&bb_hdr) < hdr_len )
        {
            return 0;
        }

        int expect = (!!has_pts) * 5 + (has_pts & 0x01) * 5 + has_escr * 6 + 
                     has_esrate * 3 + has_dsm + has_copy_info + has_crc * 2 + 
                     has_ext;

        if ( bits_bytes_left(&bb_hdr) < expect )
        {
            return 0;
        }

        if( has_pts )
        {
            if ( bits_bytes_left(&bb_hdr) < 5 )
            {
                return 0;
            }
            bits_skip(&bb_hdr, 4);
            pes_info->pts = parse_pes_timestamp( &bb_hdr );
            if ( has_pts & 1 )
            {
                if ( bits_bytes_left(&bb_hdr) < 5 )
                {
                    return 0;
                }
                bits_skip(&bb_hdr, 4);
                pes_info->dts = parse_pes_timestamp( &bb_hdr );
            }
            else
            {
                pes_info->dts = pes_info->pts;
            }
        }

        if ( has_escr )
            bits_skip(&bb_hdr, 8 * 6);
        if ( has_esrate )
            bits_skip(&bb_hdr, 8 * 3);
        if ( has_dsm )
            bits_skip(&bb_hdr, 8);
        if ( has_copy_info )
            bits_skip(&bb_hdr, 8);
        if ( has_crc )
            bits_skip(&bb_hdr, 8 * 2);

        if ( has_ext )
        {
            int has_private = bits_get(&bb_hdr, 1);
            int has_pack = bits_get(&bb_hdr, 1);
            int has_counter = bits_get(&bb_hdr, 1);
            int has_pstd = bits_get(&bb_hdr, 1);
            bits_skip(&bb_hdr, 3);   // reserved bits
            int has_ext2 = bits_get(&bb_hdr, 1);

            expect = (has_private) * 16 + has_pack + has_counter * 2 + 
                     has_pstd * 2 + has_ext2 * 2;

            if ( bits_bytes_left(&bb_hdr) < expect )
            {
                return 0;
            }

            if ( has_private )
            {
                bits_skip(&bb_hdr, 8 * 16);
                expect -= 2;
            }
            if ( has_pack )
            {
                int len = bits_get(&bb_hdr, 8);
                expect -= 1;
                if ( bits_bytes_left(&bb_hdr) < len + expect )
                {
                    return 0;
                }
                bits_skip(&bb_hdr, 8 * len);
            }
            if ( has_counter )
                bits_skip(&bb_hdr, 8 * 2);
            if ( has_pstd )
                bits_skip(&bb_hdr, 8 * 2);

            if ( has_ext2 )
            {
                bits_skip(&bb_hdr, 1);   // marker
                bits_get(&bb_hdr, 7);    // extension length
                pes_info->has_stream_id_ext = !bits_get(&bb_hdr, 1);
                if ( pes_info->has_stream_id_ext )
                    pes_info->stream_id_ext = bits_get(&bb_hdr, 7);
            }
        }
        // eat header stuffing
        bits_skip(bb, 8 * hdr_len);
    }
    else
    {
        // mpeg1 pes

        // Skip stuffing
        while ( bits_peek(bb, 1) && bits_bytes_left(bb) )
            bits_get(bb, 8);

        if ( !bits_bytes_left(bb) )
            return 0;

        // Skip std buffer info
        int mark = bits_get(bb, 2);
        if ( mark == 0x01 )
        {
            if ( bits_bytes_left(bb) < 2 )
                return 0;
            bits_skip(bb, 8 * 2);
        }

        int has_pts = bits_get(bb, 2);
        if( has_pts == 0x02 )
        {
            pes_info->pts = parse_pes_timestamp( bb );
            pes_info->dts = pes_info->pts;
        }
        else if( has_pts == 0x03 )
        {
            pes_info->pts = parse_pes_timestamp( bb );
            bits_skip(bb, 4);
            pes_info->dts = parse_pes_timestamp( bb );
        }
        else
        {
            bits_skip(bb, 8); // 0x0f flag
        }
        if ( bits_bytes_left(bb) < 0 )
            return 0;
        pes_info->header_len = bb->pos >> 3;
    }
    if ( pes_info->stream_id == 0xbd && stream->hb_stream_type == program )
    {
        if ( bits_bytes_left(bb) < 4 )
        {
            return 0;
        }
        int ssid = bits_peek(bb, 8);
        if( ( ssid >= 0xa0 && ssid <= 0xaf ) ||
            ( ssid >= 0x20 && ssid <= 0x2f ) )
        {
            // DVD LPCM or DVD SPU (subtitles)
            pes_info->bd_substream_id = bits_get(bb, 8);
            pes_info->header_len += 1;
        }
        else if ( ssid >= 0xb0 && ssid <= 0xbf )
        {
            // HD-DVD TrueHD has a 4 byte header
            pes_info->bd_substream_id = bits_get(bb, 8);
            bits_skip(bb, 8 * 4);
            pes_info->header_len += 5;
        }
        else if( ( ssid >= 0x80 && ssid <= 0x9f ) ||
                 ( ssid >= 0xc0 && ssid <= 0xcf ) )
        {
            // AC3, E-AC3, DTS, and DTS-HD has 3 byte header
            pes_info->bd_substream_id = bits_get(bb, 8);
            bits_skip(bb, 8 * 3);
            pes_info->header_len += 4;
        }
    }
    return 1;
}

static int parse_pack_header(
    hb_stream_t   *stream,
    bitbuf_t      *bb,
    hb_pes_info_t *pes_info )
{
    if ( bits_bytes_left(bb) < 12)
    {
        return 0;
    }

    bits_skip(bb, 8 * 4);
    int mark = bits_get(bb, 2);

    if ( mark == 0x00 )
    {
        // mpeg1 pack
        bits_skip(bb, 2); // marker
    }
    pes_info->scr = parse_pes_timestamp( bb );

    if ( mark == 0x00 )
    {
        bits_skip(bb, 24);
        pes_info->header_len = (bb->pos >> 3);
    }
    else
    {
        bits_skip(bb, 39);
        int stuffing = bits_get(bb, 3);
        pes_info->header_len = stuffing;
        pes_info->header_len += (bb->pos >> 3);
    }
    return 1;
}

// Returns the length of the header
static int hb_parse_ps(
    hb_stream_t   *stream,
    uint8_t       *buf,
    int           len,
    hb_pes_info_t *pes_info )
{
    memset( pes_info, 0, sizeof( hb_pes_info_t ) );
    pes_info->pts = -1;
    pes_info->dts = -1;

    bitbuf_t bb, cc;
    bits_init(&bb, buf, len, 0);
    bits_clone(&cc, &bb, len);

    if ( bits_bytes_left(&bb) < 4 )
        return 0;

    // Validate start code
    if ( bits_get(&bb, 8 * 3) != 0x000001 )
    {
        return 0;
    }

    pes_info->stream_id = bits_get(&bb, 8);
    if ( pes_info->stream_id == 0xb9 )
    {
        // Program stream end code
        return 1;
    }
    else if ( pes_info->stream_id == 0xba )
    {
        return parse_pack_header( stream, &cc, pes_info );
    }
    else if ( pes_info->stream_id >= 0xbd &&
              pes_info->stream_id != 0xbe &&
              pes_info->stream_id != 0xbf &&
              pes_info->stream_id != 0xf0 &&
              pes_info->stream_id != 0xf1 &&
              pes_info->stream_id != 0xf2 &&
              pes_info->stream_id != 0xf8 &&
              pes_info->stream_id != 0xff )
    {
        return parse_pes_header( stream, &cc, pes_info );
    }
    else
    {
        if ( bits_bytes_left(&bb) < 2 )
        {
            return 0;
        }
        pes_info->packet_len = bits_get(&bb, 16);
        pes_info->header_len = bb.pos >> 3;
        return 1;
    }
}

static int hb_ps_read_packet( hb_stream_t * stream, hb_buffer_t *b )
{
    // Appends to buffer if size != 0
    int start_code = -1;
    int pos = b->size;
    int stream_id = -1;
    int c;

#define cp (b->data)
    flockfile( stream->file_handle );
    while ( ( c = getc_unlocked( stream->file_handle ) ) != EOF )
    {
        start_code = ( start_code << 8 ) | c;
        if ( ( start_code >> 8 )== 0x000001 )
            // we found the start of the next start
            break;
    }
    if ( c == EOF )
        goto done;

    if ( pos + 4 > b->alloc )
    {
        // need to expand the buffer
        hb_buffer_realloc( b, b->alloc * 2 );
    }
    cp[pos++] = ( start_code >> 24 ) & 0xff;
    cp[pos++] = ( start_code >> 16 ) & 0xff;
    cp[pos++] = ( start_code >>  8 ) & 0xff;
    cp[pos++] = ( start_code )       & 0xff;
    stream_id = start_code & 0xff;

    if ( stream_id == 0xba )
    {
        int start = pos - 4;
        // Read pack header
        if ( pos + 21 >= b->alloc )
        {
            // need to expand the buffer
            hb_buffer_realloc( b, b->alloc * 2 );
        }

        // There are at least 8 bytes.  More if this is mpeg2 pack.
        fread( cp+pos, 1, 8, stream->file_handle );
        int mark = cp[pos] >> 4;
        pos += 8;

        if ( mark != 0x02 )
        {
            // mpeg-2 pack,
            fread( cp+pos, 1, 2, stream->file_handle );
            pos += 2;
            int len = cp[start+13] & 0x7;
            fread( cp+pos, 1, len, stream->file_handle );
            pos += len;
        }
    }
    // Non-video streams can emulate start codes, so we need
    // to inspect PES packets and skip over their data
    // sections to avoid mis-detection of the next pack or pes start code
    else if ( stream_id >= 0xbb )
    {
        int len = 0;
        c = getc_unlocked( stream->file_handle );
        if ( c == EOF )
            goto done;
        len = c << 8;
        c = getc_unlocked( stream->file_handle );
        if ( c == EOF )
            goto done;
        len |= c;
        if ( pos + len + 2 > b->alloc )
        {
            if ( b->alloc * 2 > pos + len + 2 )
                hb_buffer_realloc( b, b->alloc * 2 );
            else
                hb_buffer_realloc( b, b->alloc * 2 + len + 2 );
        }
        cp[pos++] = len >> 8;
        cp[pos++] = len & 0xff;
        if ( len )
        {
            // Length is non-zero, read the packet all at once
            len = fread( cp+pos, 1, len, stream->file_handle );
            pos += len;
        }
        else
        {
            // Length is zero, read bytes till we find a start code.
            // Only video PES packets are allowed to have zero length.
            start_code = -1;
            while ( ( c = getc_unlocked( stream->file_handle ) ) != EOF )
            {
                start_code = ( start_code << 8 ) | c;
                if ( pos  >= b->alloc )
                {
                    // need to expand the buffer
                    hb_buffer_realloc( b, b->alloc * 2 );
                }
                cp[pos++] = c;
                if ( ( start_code >> 8   ) == 0x000001 &&
                     ( start_code & 0xff ) >= 0xb9 )
                {
                    // we found the start of the next start
                    break;
                }
            }
            if ( c == EOF )
                goto done;
            pos -= 4;
            fseeko( stream->file_handle, -4, SEEK_CUR );
        }
    }
    else
    {
        // Unknown, find next start code
        start_code = -1;
        while ( ( c = getc_unlocked( stream->file_handle ) ) != EOF )
        {
            start_code = ( start_code << 8 ) | c;
            if ( pos  >= b->alloc )
            {
                // need to expand the buffer
                hb_buffer_realloc( b, b->alloc * 2 );
            }
            cp[pos++] = c;
            if ( ( start_code >> 8 ) == 0x000001 &&
                 ( start_code & 0xff ) >= 0xb9 )
                // we found the start of the next start
                break;
        }
        if ( c == EOF )
            goto done;
        pos -= 4;
        fseeko( stream->file_handle, -4, SEEK_CUR );
    }
done:
    // Parse packet for information we might need
    funlockfile( stream->file_handle );
    int len = pos - b->size;
    b->size = pos;
#undef cp
    return len;
}

static hb_buffer_t * hb_ps_stream_decode( hb_stream_t *stream )
{
    hb_pes_info_t pes_info;
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);

    while (1)
    {
        buf->size = 0;
        int len = hb_ps_read_packet( stream, buf );
        if ( len == 0 )
        {
            // End of file
            hb_buffer_close( &buf );
            return buf;
        }
        if ( !hb_parse_ps( stream, buf->data, buf->size, &pes_info ) )
        {
            ++stream->errors;
            continue;
        }
        // pack header
        if ( pes_info.stream_id == 0xba )
        {
            stream->pes.found_scr = 1;
            stream->ts_flags |= TS_HAS_PCR;
            stream->pes.scr = pes_info.scr;
            continue;
        }

        // If we don't have a SCR yet but the stream has SCRs just loop
        // so we don't process anything until we have a clock reference.
        if ( !stream->pes.found_scr && ( stream->ts_flags & TS_HAS_PCR ) )
        {
            continue;
        }

        // system header
        if ( pes_info.stream_id == 0xbb )
            continue;

        int idx;
        if ( pes_info.stream_id == 0xbd )
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.bd_substream_id );
        }
        else
        {
            idx = index_of_ps_stream( stream, pes_info.stream_id,
                                      pes_info.stream_id_ext );
        }

        // Is this a stream carrying data that we care about?
        if ( idx < 0 )
            continue;

        switch (stream->pes.list[idx].stream_kind)
        {
            case A:
                buf->type = AUDIO_BUF;
                break;

            case V:
                buf->type = VIDEO_BUF;
                break;

            default:
                buf->type = OTHER_BUF;
                break;
        }

        if ( stream->need_keyframe )
        {
            // we're looking for the first video frame because we're
            // doing random access during 'scan'
            if ( buf->type != VIDEO_BUF ||
                 !isIframe( stream, buf->data, buf->size ) )
            {
                // not the video stream or didn't find an I frame
                // but we'll only wait 255 video frames for an I frame.
                if ( buf->type != VIDEO_BUF || ++stream->need_keyframe < 512 )
                {
                    continue;
                }
            }
            stream->need_keyframe = 0;
        }
        if ( buf->type == VIDEO_BUF )
            ++stream->frames;

        buf->id = get_id( &stream->pes.list[idx] );
        buf->pcr = stream->pes.scr;
        buf->start = pes_info.pts;
        buf->renderOffset = pes_info.dts;
        memmove( buf->data, buf->data + pes_info.header_len,
                 buf->size - pes_info.header_len );
        buf->size -= pes_info.header_len;
        if ( buf->size == 0 )
            continue;
        stream->pes.scr = -1;
        return buf;
    }
}

static int update_ps_streams( hb_stream_t * stream, int stream_id, int stream_id_ext, int stream_type, int in_kind )
{
    int ii;
    int same_stream = -1;
    kind_t kind = in_kind == -1 ? st2codec[stream_type].kind : in_kind;

    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_id == stream_id )
            same_stream = ii;

        if ( stream->pes.list[ii].stream_id == stream_id &&
             stream->pes.list[ii].stream_id_ext == 0 &&
             stream->pes.list[ii].stream_kind == U )
        {
            // This is an unknown stream type that hasn't been
            // given a stream_id_ext.  So match only to stream_id
            //
            // is the stream_id_ext being updated?
            if ( stream_id_ext != 0 )
                break;

            // If stream is already in the list and the new 'kind' is
            // PCR, Unknown, or same as before, just return the index
            // to the entry found.
            if ( kind == P || kind == U || kind == stream->pes.list[ii].stream_kind )
                return ii;
            // Update stream_type and kind
            break;
        }
        if ( stream_id == stream->pes.list[ii].stream_id &&
             stream_id_ext == stream->pes.list[ii].stream_id_ext )
        {
            // If stream is already in the list and the new 'kind' is
            // PCR and the old 'kind' is unknown, set the new 'kind'
            if ( kind == P && stream->pes.list[ii].stream_kind == U )
                break;

            // If stream is already in the list and the new 'kind' is
            // PCR, Unknown, or same as before, just return the index
            // to the entry found.
            if ( kind == P || kind == U || kind == stream->pes.list[ii].stream_kind )
                return ii;
            // Replace unknown 'kind' with known 'kind'
            break;
        }
        // Resolve multiple videos
        if ( kind == V && stream->pes.list[ii].stream_kind == V )
        {
            if ( stream_id <= stream->pes.list[ii].stream_id &&
                 stream_id_ext <= stream->pes.list[ii].stream_id_ext )
            {
                // Assume primary video stream has the smallest stream id
                // and only use the primary. move the current item
                // to the end of the list.  we want to keep it for 
                // debug and informational purposes.
                int jj = new_pes( stream );
                memcpy( &stream->pes.list[jj], &stream->pes.list[ii],
                        sizeof( hb_pes_stream_t ) );
                break;
            }
        }
    }

    if ( ii == stream->pes.count )
    {
        ii = new_pes( stream );
        if ( same_stream >= 0 )
        {
            memcpy( &stream->pes.list[ii], &stream->pes.list[same_stream],
                    sizeof( hb_pes_stream_t ) );
        }
        else
        {
            stream->pes.list[ii].map_idx = -1;
        }
    }

    stream->pes.list[ii].stream_id = stream_id;
    stream->pes.list[ii].stream_id_ext = stream_id_ext;
    stream->pes.list[ii].stream_type = stream_type;
    stream->pes.list[ii].stream_kind = kind;
    return ii;
}

static void update_pes_kind( hb_stream_t * stream, int idx )
{
    kind_t kind = st2codec[stream->pes.list[idx].stream_type].kind;
    if ( kind != U && kind != N )
    {
        stream->pes.list[idx].stream_kind = kind;
    }
}

static void ts_pes_list_add( hb_stream_t *stream, int ts_idx, int pes_idx )
{
    int ii = stream->ts.list[ts_idx].pes_list;
    if ( ii == -1 )
    {
        stream->ts.list[ts_idx].pes_list = pes_idx;
        return;
    }

    int idx;
    while ( ii != -1 )
    {
        if ( ii == pes_idx ) // Already in list
            return;
        idx = ii;
        ii = stream->pes.list[ii].next;
    }
    stream->pes.list[idx].next = pes_idx;
}

static int update_ts_streams( hb_stream_t * stream, int pid, int stream_id_ext, int stream_type, int in_kind, int *out_pes_idx )
{
    int ii;
    int pes_idx = update_ps_streams( stream, pid, stream_id_ext,
                                     stream_type, in_kind );

    if ( out_pes_idx )
        *out_pes_idx = pes_idx;

    if ( pes_idx < 0 )
        return -1;

    kind_t kind = stream->pes.list[pes_idx].stream_kind;
    for ( ii = 0; ii < stream->ts.count; ii++ )
    {
        if ( pid == stream->ts.list[ii].pid )
        {
            break;
        }
        // Resolve multiple videos
        if ( kind == V && ts_stream_kind( stream, ii ) == V && 
             pes_idx < stream->ts.list[ii].pes_list )
        {
            // We have a new candidate for the primary video.  Move
            // the current video to the end of the list. And put the
            // new video in this slot
            int jj = new_pid( stream );
            memcpy( &stream->ts.list[jj], &stream->ts.list[ii], 
                    sizeof( hb_ts_stream_t ) );
            break;
        }
    }
    if ( ii == stream->ts.count )
        ii = new_pid( stream );

    stream->ts.list[ii].pid = pid;
    ts_pes_list_add( stream, ii, pes_idx );
    if ( in_kind == P )
        stream->ts.list[ii].is_pcr = 1;

    return ii;
}

static int decode_ps_map( hb_stream_t * stream, uint8_t *buf, int len )
{
    int retval = 1;
    bitbuf_t bb;
    bits_init(&bb, buf, len, 0);

    if ( bits_bytes_left(&bb) < 10 )
        return 0;

    // Skip stuff not needed
    bits_skip(&bb, 8 * 8);
    int info_len = bits_get(&bb, 16);
    if ( bits_bytes_left(&bb) < info_len )
        return 0;

    if ( info_len )
    {
        bitbuf_t cc;
        bits_clone( &cc, &bb, info_len );

        while ( bits_bytes_left(&cc) >= 2 )
        {
            uint8_t tag, len;

            tag = bits_get(&cc, 8);
            len = bits_get(&cc, 8);
            if ( bits_bytes_left(&cc) < len )
                return 0;

            if (tag == 0x05 && len >= 4)
            {
                // registration descriptor
                stream->reg_desc = bits_get(&cc, 32);
                bits_skip(&cc, 8 * (len - 4));
            }
            else
            {
                bits_skip(&cc, 8 * len);
            }
        }
        bits_skip(&bb, 8 * info_len);
    }

    int map_len = bits_get(&bb, 16);
    if ( bits_bytes_left(&bb) < map_len )
        return 0;

    // Process the map
    int ii = 0;
    while ( bits_bytes_left(&bb) >= 8 )
    {
        int pes_idx;
        int stream_type = bits_get(&bb, 8);
        int stream_id = bits_get(&bb, 8);
        info_len = bits_get(&bb, 16);
        if ( info_len > bits_bytes_left(&bb) )
            return 0;

        int substream_id = 0;
        switch ( stream_type )
        {
            case 0x81: // ac3
            case 0x82: // dts
            case 0x83: // lpcm
            case 0x87: // eac3
                // If the stream_id isn't one of the standard mpeg
                // stream ids, assume it is an private stream 1 substream id.
                // This is how most PS streams specify this type of audio.
                //
                // TiVo sets the stream id to 0xbd and does not
                // give a substream id.  This limits them to one audio 
                // stream and differs from how everyone else specifies
                // this type of audio.
                if ( stream_id < 0xb9 )
                {
                    substream_id = stream_id;
                    stream_id = 0xbd;
                }
                break;
            default:
                break;
        }

        pes_idx = update_ps_streams( stream, stream_id, substream_id,
                                     stream_type, -1 );
        if ( pes_idx >= 0 )
            stream->pes.list[pes_idx].map_idx = ii;

        if ( info_len > 0 )
        {
            bitbuf_t bb_desc;
            bits_clone( &bb_desc, &bb, info_len );
            if ( pes_idx >= 0 )
                decode_element_descriptors( stream, pes_idx, &bb_desc );
            bits_skip(&bb, 8 * info_len);
        }
        ii++;
    }
    // skip CRC 32
    return retval;
}

static void hb_ps_stream_find_streams(hb_stream_t *stream)
{
    int ii, jj;
    hb_buffer_t *buf  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);

    fseeko( stream->file_handle, 0, SEEK_SET );
    // Scan beginning of file, then if no program stream map is found
    // seek to 20% and scan again since there's occasionally no
    // audio at the beginning (particularly for vobs).
    for ( ii = 0; ii < 2; ii++ )
    {
        for ( jj = 0; jj < MAX_PS_PROBE_SIZE; jj += buf->size )
        {
            int stream_type;
            int len;

            hb_pes_info_t pes_info;
            buf->size = 0;
            len = hb_ps_read_packet( stream, buf );
            if ( len == 0 )
            {
                // Must have reached EOF
                break;
            }
            if ( !hb_parse_ps( stream, buf->data, buf->size, &pes_info ) )
            {
                hb_deep_log( 2, "hb_ps_stream_find_streams: Error parsing PS packet");
                continue;
            }
            if ( pes_info.stream_id == 0xba )
            {
                stream->ts_flags |= TS_HAS_PCR;
            }
            else if ( pes_info.stream_id == 0xbc )
            {
                // program stream map
                // Note that if there is a program map, any
                // extrapolation that is made below based on 
                // stream id may be overridden by entry in the map.
                if ( decode_ps_map( stream, buf->data, buf->size ) )
                {
                    hb_log("Found program stream map");
                    goto done;
                }
                else
                {
                    hb_error("Error parsing program stream map");
                }
            }
            else if ( ( pes_info.stream_id & 0xe0 ) == 0xc0 )
            {
                // MPeg audio (c0 - df)
                stream_type = 0x04;
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.stream_id_ext, stream_type, -1 );
            }
            else if ( pes_info.stream_id == 0xbd )
            {
                int ssid = pes_info.bd_substream_id;
                // Add a potentail audio stream
                // Check dvd substream id
                if ( ssid >= 0x20 && ssid <= 0x37 )
                {
                    // Skip dvd subtitles
                    int idx = update_ps_streams( stream, pes_info.stream_id,
                                            pes_info.bd_substream_id, 0, -1 );
                    stream->pes.list[idx].stream_kind = N;
                    strncpy(stream->pes.list[idx].codec_name, 
                            "DVD Subtitle", 80);
                    continue;
                }
                if ( ssid >= 0x80 && ssid <= 0x87 )
                {
                    stream_type = 0x81; // ac3
                }
                else if ( ( ssid >= 0x88 && ssid <= 0x8f ) ||
                          ( ssid >= 0x98 && ssid <= 0x9f ) )
                {
                    // Could be either dts or dts-hd
                    // will have to probe to resolve
                    int idx = update_ps_streams( stream, pes_info.stream_id,
                                            pes_info.bd_substream_id, 0, U );
                    stream->pes.list[idx].codec = HB_ACODEC_DCA_HD;
                    stream->pes.list[idx].codec_param = CODEC_ID_DTS;
                    continue;
                }
                else if ( ssid >= 0xa0 && ssid <= 0xaf )
                {
                    stream_type = 0x83; // lpcm
                    // This is flagged as an unknown stream type in
                    // st2codec because it can be either LPCM or
                    // BD TrueHD. In this case it is LPCM.
                    update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.bd_substream_id, stream_type, A );
                    continue;
                }
                else if ( ssid >= 0xb0 && ssid <= 0xbf )
                {
                    // HD-DVD TrueHD
                    int idx = update_ps_streams( stream, pes_info.stream_id,
                                            pes_info.bd_substream_id, 0, A );
                    stream->pes.list[idx].codec = HB_ACODEC_FFMPEG;
                    stream->pes.list[idx].codec_param = CODEC_ID_TRUEHD;
                    continue;
                }
                else if ( ssid >= 0xc0 && ssid <= 0xcf )
                {
                    // HD-DVD uses this for both ac3 and eac3.
                    // Check ac3 bitstream_id to distinguish between them.
                    bitbuf_t bb;
                    bits_init(&bb, buf->data + pes_info.header_len, 
                              buf->size - pes_info.header_len, 0);
                    int sync = bits_get(&bb, 16);
                    if ( sync == 0x0b77 )
                    {
                        bits_skip(&bb, 24);
                        int bsid = bits_get(&bb, 5);
                        if ( bsid <= 10 )
                        {
                            // ac3
                            stream_type = 0x81; // ac3
                        }
                        else
                        {
                            // eac3
                            stream_type = 0x87; // eac3
                        }
                    }
                    else
                    {
                        // Doesn't look like an ac3 stream.  Probe it.
                        stream_type = 0x00;
                    }
                }
                else
                {
                    // Unknown. Probe it.
                    stream_type = 0x00;
                }
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.bd_substream_id, stream_type, -1 );
            }
            else if ( ( pes_info.stream_id & 0xf0 ) == 0xe0 )
            {
                // Normally this is MPEG video, but MPEG-1 PS streams
                // (which do not have a program stream map)  may use 
                // this for other types of video.
                //
                // Also, the hddvd tards decided to use 0xe2 and 0xe3 for
                // h.264 video :( and the twits decided not to put a
                // program stream map in the stream :'(
                //
                // So set this to an unknown stream type and probe.
                stream_type = 0x00;
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.stream_id_ext, stream_type, -1 );
            }
            else if ( pes_info.stream_id == 0xfd )
            {
                if ( pes_info.stream_id_ext == 0x55 ||
                     pes_info.stream_id_ext == 0x56 )
                {
                    // hddvd uses this for vc-1.
                    stream_type = 0xea;
                }
                else
                {  
                    // mark as unknown and probe.
                    stream_type = 0x00;
                }
                update_ps_streams( stream, pes_info.stream_id,
                                   pes_info.stream_id_ext, stream_type, -1 );
            }
        }
        hb_stream_seek( stream, 0.2 );
    }
done:
    hb_buffer_close( &buf );
}

static int probe_dts_profile( hb_pes_stream_t *pes )
{
    hb_work_info_t info;
    hb_work_object_t *w = hb_codec_decoder( pes->codec );

    w->codec_param = pes->codec_param;
    int ret = w->bsinfo( w, pes->probe_buf, &info );
    if ( ret < 0 )
    {
        hb_log( "probe_dts_profile: no info type %d/0x%x for id 0x%x",
                pes->codec, pes->codec_param, pes->stream_id );

    }
    switch (info.profile)
    {
        case FF_PROFILE_DTS:
        case FF_PROFILE_DTS_96_24:
            pes->codec = HB_ACODEC_DCA;
            pes->stream_type = 0x82;
            pes->stream_kind = A;
            break;

        case FF_PROFILE_DTS_ES:
            pes->stream_type = 0;
            pes->stream_kind = A;
            break;

        case FF_PROFILE_DTS_HD_HRA:
            pes->stream_type = 0;
            pes->stream_kind = A;
            break;

        case FF_PROFILE_DTS_HD_MA:
            pes->stream_type = 0;
            pes->stream_kind = A;
            break;

        default:
            return 0;
    }
    const char *profile_name;
    AVCodec *codec = avcodec_find_decoder( pes->codec_param );
    profile_name = av_get_profile_name( codec, info.profile );
    if ( profile_name )
    {
        strncpy(pes->codec_name, profile_name, 80);
        pes->codec_name[79] = 0;
    }
    return 1;
}

static int do_probe( hb_pes_stream_t *pes, hb_buffer_t *buf )
{
    // Check upper limit of per stream data to probe
    if ( pes->probe_buf == NULL )
    {
        pes->probe_buf = hb_buffer_init( 0 );
    }
    if ( pes->probe_buf->size > HB_MAX_PROBE_SIZE )
    {
        pes->stream_kind = N;
        hb_buffer_close( &pes->probe_buf );
        return 1;
    }

    // Add this stream buffer to probe buffer and perform probe
    AVInputFormat *fmt = NULL;
    int score = 0;
    AVProbeData pd = {0,};
    int size = pes->probe_buf->size + buf->size;

    hb_buffer_realloc(pes->probe_buf, size + AVPROBE_PADDING_SIZE );
    memcpy( pes->probe_buf->data + pes->probe_buf->size, buf->data, buf->size );
    pes->probe_buf->size = size;

    if ( pes->codec == HB_ACODEC_DCA_HD )
    {
        // We need to probe for the profile of DTS audio in this stream.
        return probe_dts_profile( pes );
    }

    // Probing is slow, so we don't want to re-probe the probe
    // buffer for every packet we add to it.  Grow the buffer
    // by a factor of 2 before probing again.
    if ( pes->probe_buf->size < pes->probe_next_size )
        return 0;

    pes->probe_next_size = pes->probe_buf->size * 2;
    pd.buf = pes->probe_buf->data;
    pd.buf_size = pes->probe_buf->size;
    fmt = av_probe_input_format2( &pd, 1, &score );
    if ( fmt && score > AVPROBE_SCORE_MAX / 2 )
    {
        AVCodec *codec = avcodec_find_decoder_by_name( fmt->name );
        if( !codec )
        {
            int i;
            static const struct {
                const char *name; enum CodecID id;
            } fmt_id_type[] = {
                { "g722"     , CODEC_ID_ADPCM_G722 },
                { "mlp"      , CODEC_ID_MLP        },
                { "truehd"   , CODEC_ID_TRUEHD     },
                { "shn"      , CODEC_ID_SHORTEN    },
                { "aac"      , CODEC_ID_AAC        },
                { "ac3"      , CODEC_ID_AC3        },
                { "dts"      , CODEC_ID_DTS        },
                { "eac3"     , CODEC_ID_EAC3       },
                { "h264"     , CODEC_ID_H264       },
                { "m4v"      , CODEC_ID_MPEG4      },
                { "mp3"      , CODEC_ID_MP3        },
                { "mpegvideo", CODEC_ID_MPEG2VIDEO },
                { "cavsvideo", CODEC_ID_CAVS },
                { "dnxhd"    , CODEC_ID_DNXHD },
                { "h261"     , CODEC_ID_H261 },
                { "h263"     , CODEC_ID_H263 },
                { "mjpeg"    , CODEC_ID_MJPEG },
                { "vc1"      , CODEC_ID_VC1 },
                { 0 }
            };
            for( i = 0; fmt_id_type[i].name; i++ )
            {
                if( !strcmp(fmt->name, fmt_id_type[i].name ) )
                {
                    codec = avcodec_find_decoder( fmt_id_type[i].id );
                    break;
                }
            }
        }
        if( codec )
        {
            pes->codec_param = codec->id;
            if ( codec->type == AVMEDIA_TYPE_VIDEO )
            {
                pes->stream_kind = V;
                switch ( codec->id )
                {
                    case CODEC_ID_MPEG1VIDEO:
                        pes->codec = WORK_DECMPEG2;
                        pes->stream_type = 0x01;
                        break;

                    case CODEC_ID_MPEG2VIDEO:
                        pes->codec = WORK_DECMPEG2;
                        pes->stream_type = 0x02;
                        break;

                    case CODEC_ID_H264:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_type = 0x1b;
                        break;

                    case CODEC_ID_VC1:
                        pes->codec = WORK_DECAVCODECV;
                        pes->stream_type = 0xea;
                        break;

                    default:
                        pes->codec = WORK_DECAVCODECV;
                }
            }
            else if ( codec->type == AVMEDIA_TYPE_AUDIO )
            {
                pes->stream_kind = A;
                switch ( codec->id )
                {
                    case CODEC_ID_AC3:
                        pes->codec = HB_ACODEC_AC3;
                        break;
                    default:
                        pes->codec = HB_ACODEC_FFMPEG;
                }
            }
            else
            {
                pes->stream_kind = N;
            }
            strncpy(pes->codec_name, codec->name, 79);
            pes->codec_name[79] = 0;
        }
        else
        {
            pes->stream_kind = N;
        }
        hb_buffer_close( &pes->probe_buf );
        return 1;
    }
    return 0;
}

static void hb_ts_resolve_pid_types(hb_stream_t *stream)
{
    int ii, probe = 0;

    for ( ii = 0; ii < stream->ts.count; ii++ )
    {
        int pid = stream->ts.list[ii].pid;
        int stype = ts_stream_type( stream, ii );
        int pes_idx;

        if ( stype == 0x80 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // LPCM audio in bluray have an stype of 0x80
            // 0x80 is used for other DigiCipher normally
            // To distinguish, Bluray streams have a reg_desc of HDMV
            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_FFMPEG;
            stream->pes.list[pes_idx].codec_param = CODEC_ID_PCM_BLURAY;
            continue;
        }

        // The blu ray consortium apparently forgot to read the portion
        // of the MPEG spec that says one PID should map to one media
        // stream and multiplexed multiple types of audio into one PID
        // using the extended stream identifier of the PES header to
        // distinguish them. So we have to check if that's happening and
        // if so tell the runtime what esid we want.
        if ( stype == 0x83 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // This is an interleaved TrueHD/AC-3 stream and the esid of
            // the AC-3 is 0x76
            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_AC3,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_AC3;

            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_TRUEHD,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_FFMPEG;
            stream->pes.list[pes_idx].codec_param = CODEC_ID_TRUEHD;
            continue;
        }
        if ( ( stype == 0x84 || stype == 0xa1 ) &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // EAC3 audio in bluray has an stype of 0x84
            // which conflicts with SDDS
            // To distinguish, Bluray streams have a reg_desc of HDMV
            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_FFMPEG;
            stream->pes.list[pes_idx].codec_param = CODEC_ID_EAC3;
            continue;
        }
        // 0xa2 is DTS-HD LBR used in HD-DVD and bluray for
        // secondary audio streams. Libav can not decode yet.
        // Having it in the audio list causes delays during scan
        // while we try to get stream parameters. So skip 
        // this type for now.
        if ( stype == 0x85 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // DTS-HD HRA audio in bluray has an stype of 0x85
            // which conflicts with ATSC Program ID
            // To distinguish, Bluray streams have a reg_desc of HDMV
            // This is an interleaved DTS-HD HRA/DTS stream and the
            // esid of the DTS is 0x71
            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_DTS,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA;

            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA_HD;
            stream->pes.list[pes_idx].codec_param = CODEC_ID_DTS;
            continue;
        }
        if ( stype == 0x86 &&
             stream->reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // This is an interleaved DTS-HD MA/DTS stream and the
            // esid of the DTS is 0x71
            update_ts_streams( stream, pid, HB_SUBSTREAM_BD_DTS,
                               stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA;

            update_ts_streams( stream, pid, 0, stype, A, &pes_idx );
            stream->pes.list[pes_idx].codec = HB_ACODEC_DCA_HD;
            stream->pes.list[pes_idx].codec_param = CODEC_ID_DTS;
            continue;
        }

        // stype == 0 indicates a type not in st2codec table
        if ( stype != 0 &&
             ( ts_stream_kind( stream, ii ) == A ||
               ts_stream_kind( stream, ii ) == V ) )
        {
            // Assuming there are no substreams.
            // This should be true before probing.
            // This function is only called before
            // probing.
            pes_idx = stream->ts.list[ii].pes_list;
            stream->pes.list[pes_idx].codec = st2codec[stype].codec;
            stream->pes.list[pes_idx].codec_param = st2codec[stype].codec_param;
            continue;
        }

        if ( ts_stream_kind( stream, ii ) == U )
        {
            probe++;
        }
    }

    // Probe remaining unknown streams for stream types
    hb_stream_seek( stream, 0.0 );
    stream->need_keyframe = 0;

    int total_size = 0;
    hb_buffer_t *buf;

    if ( probe )
        hb_log("Probing %d unknown stream%s", probe, probe > 1 ? "s" : "" );

    while ( probe && ( buf = hb_ts_stream_decode( stream ) ) != NULL )
    {
        // Check upper limit of total data to probe
        total_size += buf->size;

        if ( total_size > HB_MAX_PROBE_SIZE * 2 )
            break;

        int idx;
        idx = index_of_id( stream, buf->id );

        if (idx < 0 || stream->pes.list[idx].stream_kind != U )
            continue;

        hb_pes_stream_t *pes = &stream->pes.list[idx];

        if ( do_probe( pes, buf ) )
        {
            probe--;
            if ( pes->stream_kind != N )
            {
                hb_log("    Probe: Found stream %s. stream id 0x%x-0x%x",
                        pes->codec_name, pes->stream_id, pes->stream_id_ext);
            }
            else
            {
                hb_log("    Probe: Unsupported stream %s. stream id 0x%x-0x%x",
                        pes->codec_name, pes->stream_id, pes->stream_id_ext);
            }
        }
    }
    // Clean up any probe buffers and set all remaining unknown
    // streams to 'kind' N
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == U )
            stream->pes.list[ii].stream_kind = N;
        hb_buffer_close( &stream->pes.list[ii].probe_buf );
        stream->pes.list[ii].probe_next_size = 0;
    }
}

static void hb_ps_resolve_stream_types(hb_stream_t *stream)
{
    int ii, probe = 0;

    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        int stype = stream->pes.list[ii].stream_type;

        // stype == 0 indicates a type not in st2codec table
        if ( stype != 0 &&
             ( stream->pes.list[ii].stream_kind == A ||
               stream->pes.list[ii].stream_kind == V ) )
        {
            stream->pes.list[ii].codec = st2codec[stype].codec;
            stream->pes.list[ii].codec_param = st2codec[stype].codec_param;
            continue;
        }

        if ( stream->pes.list[ii].stream_kind == U )
        {
            probe++;
        }
    }

    // Probe remaining unknown streams for stream types
    hb_stream_seek( stream, 0.0 );
    stream->need_keyframe = 0;

    int total_size = 0;
    hb_buffer_t *buf;

    if ( probe )
        hb_log("Probing %d unknown stream%s", probe, probe > 1 ? "s" : "" );

    while ( probe && ( buf = hb_ps_stream_decode( stream ) ) != NULL )
    {
        // Check upper limit of total data to probe
        total_size += buf->size;

        if ( total_size > HB_MAX_PROBE_SIZE * 2 )
            break;

        int idx;
        idx = index_of_id( stream, buf->id );

        if (idx < 0 || stream->pes.list[idx].stream_kind != U )
            continue;

        hb_pes_stream_t *pes = &stream->pes.list[idx];

        if ( do_probe( pes, buf ) )
        {
            probe--;
            if ( pes->stream_kind != N )
            {
                hb_log("    Probe: Found stream %s. stream id 0x%x-0x%x",
                        pes->codec_name, pes->stream_id, pes->stream_id_ext);
            }
            else
            {
                hb_log("    Probe: Unsupported stream %s. stream id 0x%x-0x%x",
                        pes->codec_name, pes->stream_id, pes->stream_id_ext);
            }
        }
    }
    // Clean up any probe buffers and set all remaining unknown
    // streams to 'kind' N
    for ( ii = 0; ii < stream->pes.count; ii++ )
    {
        if ( stream->pes.list[ii].stream_kind == U )
            stream->pes.list[ii].stream_kind = N;
        hb_buffer_close( &stream->pes.list[ii].probe_buf );
        stream->pes.list[ii].probe_next_size = 0;
    }
}


static void hb_ts_stream_find_pids(hb_stream_t *stream)
{
    // To be different from every other broadcaster in the world, New Zealand TV
    // changes PMTs (and thus video & audio PIDs) when 'programs' change. Since
    // we may have the tail of the previous program at the beginning of this
    // file, take our PMT from the middle of the file.
    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    fseeko(stream->file_handle, fsize >> 1, SEEK_SET);
    align_to_next_packet(stream);

    // Read the Transport Stream Packets (188 bytes each) looking at first for PID 0 (the PAT PID), then decode that
    // to find the program map PID and then decode that to get the list of audio and video PIDs

    for (;;)
    {
        const uint8_t *buf = next_packet( stream );

        if ( buf == NULL )
        {
            hb_log("hb_ts_stream_find_pids - end of file");
            break;
        }

        // Get pid
        int pid = (((buf[1] & 0x1F) << 8) | buf[2]) & 0x1FFF;

        if ((pid == 0x0000) && (stream->ts_number_pat_entries == 0))
        {
          decode_PAT(buf, stream);
          continue;
        }

        int pat_index = 0;
        for (pat_index = 0; pat_index < stream->ts_number_pat_entries; pat_index++)
        {
            // There are some streams where the PAT table has multiple
            // entries as if their are multiple programs in the same
            // transport stream, and yet there's actually only one
            // program really in the stream. This seems to be true for
            // transport streams that originate in the HDHomeRun but have
            // been output by EyeTV's export utility. What I think is
            // happening is that the HDHomeRun is sending the entire
            // transport stream as broadcast, but the EyeTV is only
            // recording a single (selected) program number and not
            // rewriting the PAT info on export to match what's actually
            // on the stream. Until we have a way of handling multiple
            // programs per transport stream elegantly we'll match on the
            // first pat entry for which we find a matching program map PID.
            // The ideal solution would be to build a title choice popup
            // from the PAT program number details and then select from
            // their - but right now the API's not capable of that.
            if (stream->pat_info[pat_index].program_number != 0 &&
                pid == stream->pat_info[pat_index].program_map_PID)
            {
              if (build_program_map(buf, stream) > 0)
                break;
            }
        }
        // Keep going  until we have a complete set of PIDs
        if ( ts_index_of_video( stream ) >= 0 )
          break;
    }
    update_ts_streams( stream, stream->pmt_info.PCR_PID, 0, -1, P, NULL );
 }


// convert a PES PTS or DTS to an int64
static int64_t pes_timestamp( const uint8_t *buf )
{
    int64_t ts;

    ts = ( (uint64_t)  ( buf[0] & 0x0e ) << 29 ) +
                       ( buf[1] <<  22 )         +
                     ( ( buf[2] >>   1 ) << 15 ) +
                       ( buf[3] <<   7 )         +
                       ( buf[4] >>   1 );
    return ts;
}

static hb_buffer_t * generate_output_data(hb_stream_t *stream, int curstream)
{
    hb_buffer_t *buf = NULL, *first = NULL;
    hb_pes_info_t pes_info;

    hb_buffer_t * b = stream->ts.list[curstream].buf;
    if ( !hb_parse_ps( stream, b->data, b->size, &pes_info ) )
    {
        b->size = 0;
        return NULL;
    }

    uint8_t *tdat = b->data + pes_info.header_len;
    int size = b->size - pes_info.header_len;

    if ( size <= 0 )
    {
        b->size = 0;
        return NULL;
    }

    int pes_idx;
    pes_idx = stream->ts.list[curstream].pes_list;
    if( stream->need_keyframe )
    {
        // we're looking for the first video frame because we're
        // doing random access during 'scan'
        int kind = stream->pes.list[pes_idx].stream_kind;
        if( kind != V || !isIframe( stream, tdat, size ) )
        {
            // not the video stream or didn't find an I frame
            // but we'll only wait 255 video frames for an I frame.
            if ( kind != V || ++stream->need_keyframe < 512 )
            {
                b->size = 0;
                return NULL;
            }
        }
        stream->need_keyframe = 0;
    }

    // Check all substreams to see if this packet matches
    for ( pes_idx = stream->ts.list[curstream].pes_list; pes_idx != -1;
          pes_idx = stream->pes.list[pes_idx].next )
    {
        if ( stream->pes.list[pes_idx].stream_id_ext != pes_info.stream_id_ext &&
             stream->pes.list[pes_idx].stream_id_ext != 0 )
        {
            continue;
        }
        // The substreams match.
        // Note that when stream->pes.list[pes_idx].stream_id_ext == 0,
        // we want the whole TS stream including all substreams.
        // DTS-HD is an example of this.

        if ( first == NULL )
            first = buf = hb_buffer_init( size );
        else
        {
            hb_buffer_t *tmp = hb_buffer_init( size );
            buf->next = tmp;
            buf = tmp;
        }

        buf->id = get_id( &stream->pes.list[pes_idx] );
        switch (stream->pes.list[pes_idx].stream_kind)
        {
            case A:
                buf->type = AUDIO_BUF;
                break;

            case V:
                buf->type = VIDEO_BUF;
                break;

            default:
                buf->type = OTHER_BUF;
                break;
        }

        if( b->cur > stream->ts.pcr_out )
        {
            // we have a new pcr
            stream->ts.pcr_out = b->cur;
            buf->pcr = b->pcr;
            if( b->cur >= stream->ts.pcr_discontinuity )
                stream->ts.pcr_current = stream->ts.pcr_discontinuity;
        }
        else
        {
            buf->pcr = -1;
        }

        // check if this packet was referenced to an older pcr and if that
        // pcr was prior to a discontinuity.
        if( b->cur < stream->ts.pcr_current )
        {
            // we've sent up a new pcr but have a packet referenced to an
            // old pcr and the difference was enough to trigger a discontinuity
            // correction. smash the timestamps or we'll mess up the correction.
            buf->start = -1;
            buf->renderOffset = -1;
            buf->stop = -1;
            buf->pcr = -1;
        }
        else
        {
            // put the PTS & possible DTS into 'start' & 'renderOffset'
            // then strip off the PES header.
            buf->start = pes_info.pts;
            buf->renderOffset = pes_info.dts;
        }
        memcpy( buf->data, tdat, size );
    }

    b->size = 0;
    return first;
}

static void hb_ts_stream_append_pkt(hb_stream_t *stream, int idx, const uint8_t *buf, int len)
{
    if (stream->ts.list[idx].buf->size + len > stream->ts.list[idx].buf->alloc)
    {
        int size;

        size = MAX( stream->ts.list[idx].buf->alloc * 2,
                    stream->ts.list[idx].buf->size + len);
        hb_buffer_realloc(stream->ts.list[idx].buf, size);
    }
    memcpy( stream->ts.list[idx].buf->data + stream->ts.list[idx].buf->size,
            buf, len);
    stream->ts.list[idx].buf->size += len;
}

/***********************************************************************
 * hb_ts_stream_decode
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_ts_decode_pkt( hb_stream_t *stream, const uint8_t * pkt )
{
    /*
     * stash the output buffer pointer in our stream so we don't have to
     * pass it & its original value to everything we call.
     */
    int video_index = ts_index_of_video(stream);
    int curstream;
    hb_buffer_t *buf;

    /* This next section validates the packet */

    // Get pid and use it to find stream state.
    int pid = ((pkt[1] & 0x1F) << 8) | pkt[2];
    if ( ( curstream = index_of_pid( stream, pid ) ) < 0 )
    {
        return NULL;
    }

    // Get error
    int errorbit = (pkt[1] & 0x80) != 0;
    if (errorbit)
    {
        ts_err( stream, curstream,  "packet error bit set");
        return NULL;
    }

    // Get adaption header info
    int adaption = (pkt[3] & 0x30) >> 4;
    int adapt_len = 0;
    if (adaption == 0)
    {
        ts_err( stream, curstream,  "adaptation code 0");
        return NULL;
    }
    else if (adaption == 0x2)
        adapt_len = 184;
    else if (adaption == 0x3)
    {
        adapt_len = pkt[4] + 1;
        if (adapt_len > 184)
        {
            ts_err( stream, curstream,  "invalid adapt len %d", adapt_len);
            return NULL;
        }
    }

    if ( adapt_len > 0 )
    {
        if ( pkt[5] & 0x40 )
        {
            // found a random access point
        }
        // if there's an adaptation header & PCR_flag is set
        // get the PCR (Program Clock Reference)
        if ( adapt_len > 7 && ( pkt[5] & 0x10 ) != 0 )
        {
            int64_t pcr;
            pcr =    ( (uint64_t)pkt[6] << (33 - 8) ) |
                     ( (uint64_t)pkt[7] << (33 - 16) ) |
                     ( (uint64_t)pkt[8] << (33 - 24) ) |
                     ( (uint64_t)pkt[9] << (33 - 32) ) |
                     ( pkt[10] >> 7 );
            ++stream->ts.pcr_in;
            stream->ts.found_pcr = 1;
            stream->ts_flags |= TS_HAS_PCR;
            // Check for a pcr discontinuity.
            // The reason for the uint cast on the pcr difference is that the
            // difference is significant if it advanced by more than 200ms or
            // if it went backwards by any amount. The negative numbers look
            // like huge unsigned ints so the cast allows both conditions to
            // be checked at once.
            if ( (uint64_t)( pcr - stream->ts.pcr ) > 200*90LL )
            {
                stream->ts.pcr_discontinuity = stream->ts.pcr_in;
            }
            stream->ts.pcr = pcr;
        }
    }

    // If we don't have a PCR yet but the stream has PCRs just loop
    // so we don't process anything until we have a clock reference.
    // Unfortunately the HD Home Run appears to null out the PCR so if
    // we didn't detect a PCR during scan keep going and we'll use
    // the video stream DTS for the PCR.
    if ( !stream->ts.found_pcr && ( stream->ts_flags & TS_HAS_PCR ) )
    {
        return NULL;
    }

    // Get continuity
    // Continuity only increments for adaption values of 0x3 or 0x01
    // and is not checked for start packets.

    int start = (pkt[1] & 0x40) != 0;

    if ( (adaption & 0x01) != 0 )
    {
        int continuity = (pkt[3] & 0xF);
        if ( continuity == stream->ts.list[curstream].continuity )
        {
            // Spliced transport streams can have duplicate
            // continuity counts at the splice boundary.
            // Test to see if the packet is really a duplicate
            // by comparing packet summaries to see if they
            // match.
            uint8_t summary[8];

            summary[0] = adaption;
            summary[1] = adapt_len;
            if (adapt_len + 4 + 6 + 9 <= 188)
            {
                memcpy(&summary[2], pkt+4+adapt_len+9, 6);
            }
            else
            {
                memset(&summary[2], 0, 6);
            }
            if ( memcmp( summary, stream->ts.list[curstream].pkt_summary, 8 ) == 0 )
            {
                // we got a duplicate packet (usually used to introduce
                // a PCR when one is needed). The only thing that can
                // change in the dup is the PCR which we grabbed above
                // so ignore the rest.
                return NULL;
            }
        }
        if ( !start && (stream->ts.list[curstream].continuity != -1) &&
             !stream->ts.list[curstream].skipbad &&
             (continuity != ( (stream->ts.list[curstream].continuity + 1) & 0xf ) ) )
        {
            ts_err( stream, curstream, "continuity error: got %d expected %d",
                    (int)continuity,
                    (stream->ts.list[curstream].continuity + 1) & 0xf );
            stream->ts.list[curstream].continuity = continuity;
            return NULL;
        }
        stream->ts.list[curstream].continuity = continuity;

        // Save a summary of this packet for later duplicate
        // testing.  The summary includes some header information
        // and payload bytes.  Should be enough to detect
        // non-duplicates.
        stream->ts.list[curstream].pkt_summary[0] = adaption;
        stream->ts.list[curstream].pkt_summary[1] = adapt_len;
        if (adapt_len + 4 + 6 + 9 <= 188)
        {
            memcpy(&stream->ts.list[curstream].pkt_summary[2],
                    pkt+4+adapt_len+9, 6);
        }
        else
        {
            memset(&stream->ts.list[curstream].pkt_summary[2], 0, 6);
        }
    }

    /* If we get here the packet is valid - process its data */


    if ( start )
    {
        // Found a random access point or we have finished generating a PES
        // and must start a new one.

        // PES must begin with an mpeg start code
        const uint8_t *pes = pkt + adapt_len + 4;
        if ( pes[0] != 0x00 || pes[1] != 0x00 || pes[2] != 0x01 )
        {
            ts_err( stream, curstream, "missing start code" );
            stream->ts.list[curstream].skipbad = 1;
            return NULL;
        }

        // If we were skipping a bad packet, start fresh on this new PES packet
        if (stream->ts.list[curstream].skipbad == 1)
        {
            stream->ts.list[curstream].skipbad = 0;
        }

        if ( curstream == video_index )
        {
            ++stream->frames;

            // if we don't have a pcr yet use the dts from this frame
            // to attempt to detect discontinuities
            if ( !stream->ts.found_pcr )
            {
                // PES must begin with an mpeg start code & contain
                // a DTS or PTS.
                const uint8_t *pes = pkt + adapt_len + 4;
                if ( pes[0] != 0x00 || pes[1] != 0x00 || pes[2] != 0x01 ||
                     ( pes[7] >> 6 ) == 0 )
                {
                    return NULL;
                }
                // if we have a dts use it otherwise use the pts
                int64_t timestamp;
                timestamp = pes_timestamp( pes + ( pes[7] & 0x40?14:9 ) );
                if( stream->ts.last_timestamp < 0 ||
                    timestamp - stream->ts.last_timestamp > 90 * 600 ||
                    stream->ts.last_timestamp - timestamp > 90 * 600 )
                {
                    stream->ts.pcr = timestamp;
                    ++stream->ts.pcr_in;
                    stream->ts.pcr_discontinuity = stream->ts.pcr_in;
                }
                stream->ts.last_timestamp = timestamp;
            }
        }

        // If we have some data already on this stream, turn it into
        // a program stream packet. Then add the payload for this
        // packet to the current pid's buffer.
        if ( stream->ts.list[curstream].buf->size )
        {
            // we have to ship the old packet before updating the pcr
            // since the packet we've been accumulating is referenced
            // to the old pcr.
            buf = generate_output_data(stream, curstream);

            if ( buf )
            {
                // Output data is ready.
                // remember the pcr that was in effect when we started
                // this packet.
                stream->ts.list[curstream].buf->cur = stream->ts.pcr_in;
                stream->ts.list[curstream].buf->pcr = stream->ts.pcr;
                hb_ts_stream_append_pkt(stream, curstream, pkt + 4 + adapt_len,
                                        184 - adapt_len);
                return buf;
            }
        }
        // remember the pcr that was in effect when we started this packet.
        stream->ts.list[curstream].buf->cur = stream->ts.pcr_in;
        stream->ts.list[curstream].buf->pcr = stream->ts.pcr;
    }

    // Add the payload for this packet to the current buffer
    if (!stream->ts.list[curstream].skipbad && (184 - adapt_len) > 0)
    {
        hb_ts_stream_append_pkt(stream, curstream, pkt + 4 + adapt_len,
                                184 - adapt_len);
        // see if we've hit the end of this PES packet
        const uint8_t *pes = stream->ts.list[curstream].buf->data;
        int len = ( pes[4] << 8 ) + pes[5] + 6;
        if ( len > 6 && stream->ts.list[curstream].buf->size == len &&
             pes[0] == 0x00 && pes[1] == 0x00 && pes[2] == 0x01 )
        {
            buf = generate_output_data(stream, curstream);
            if ( buf )
                return buf;
        }
    }
    return NULL;
}

static hb_buffer_t * hb_ts_stream_decode( hb_stream_t *stream )
{
    hb_buffer_t * b;

    // spin until we get a packet of data from some stream or hit eof
    while ( 1 )
    {
        const uint8_t *buf = next_packet(stream);
        if ( buf == NULL )
        {
            // end of file - we didn't finish filling our ps write buffer
            // so just discard the remainder (the partial buffer is useless)
            hb_log("hb_ts_stream_decode - eof");
            return NULL;
        }

        b = hb_ts_decode_pkt( stream, buf );
        if ( b )
        {
            return b;
        }
    }
    return NULL;
}

void hb_ts_stream_reset(hb_stream_t *stream)
{
    int i;

    for (i=0; i < stream->ts.count; i++)
    {
        if ( stream->ts.list[i].buf )
            stream->ts.list[i].buf->size = 0;
        if ( stream->ts.list[i].extra_buf )
            stream->ts.list[i].extra_buf->size = 0;
        stream->ts.list[i].skipbad = 1;
        stream->ts.list[i].continuity = -1;
    }

    stream->need_keyframe = 1;

    stream->ts.found_pcr = 0;
    stream->ts.pcr_out = 0;
    stream->ts.pcr_in = 0;
    stream->ts.pcr = -1;
    stream->ts.pcr_current = -1;
    stream->ts.last_timestamp = -1;

    stream->frames = 0;
    stream->errors = 0;
    stream->last_error_frame = -10000;
    stream->last_error_count = 0;
}

void hb_ps_stream_reset(hb_stream_t *stream)
{
    stream->need_keyframe = 1;

    stream->pes.found_scr = 0;
    stream->pes.scr = -1;

    stream->frames = 0;
    stream->errors = 0;
}

// ------------------------------------------------------------------
// Support for reading media files via the ffmpeg libraries.

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title, int scan )
{
    AVFormatContext *info_ic = NULL;

    av_log_set_level( AV_LOG_ERROR );

    // FFMpeg has issues with seeking.  After av_find_stream_info, the
    // streams are left in an indeterminate position.  So a seek is
    // necessary to force things back to the beginning of the stream.
    // But then the seek fails for some stream types.  So the safest thing
    // to do seems to be to open 2 AVFormatContext.  One for probing info
    // and the other for reading.
    if ( avformat_open_input( &info_ic, stream->path, NULL, NULL ) < 0 )
    {
        return 0;
    }
    if ( avformat_find_stream_info( info_ic, NULL ) < 0 )
        goto fail;

    title->opaque_priv = (void*)info_ic;
    stream->ffmpeg_ic = info_ic;
    stream->hb_stream_type = ffmpeg;
    stream->ffmpeg_pkt = malloc(sizeof(*stream->ffmpeg_pkt));
    av_init_packet( stream->ffmpeg_pkt );
    stream->chapter_end = INT64_MAX;

    if ( !scan )
    {
        // we're opening for read. scan passed out codec params that
        // indexed its stream so we need to remap them so they point
        // to this stream.
        stream->ffmpeg_video_id = title->video_id;
        av_log_set_level( AV_LOG_ERROR );
    }
    else
    {
        // we're opening for scan. let ffmpeg put some info into the
        // log about what we've got.
        stream->ffmpeg_video_id = title->video_id;
        av_log_set_level( AV_LOG_INFO );
        av_dump_format( info_ic, 0, stream->path, 0 );
        av_log_set_level( AV_LOG_ERROR );

        // accept this file if it has at least one video stream we can decode
        int i;
        for (i = 0; i < info_ic->nb_streams; ++i )
        {
            if ( info_ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
            {
                break;
            }
        }
        if ( i >= info_ic->nb_streams )
            goto fail;
    }
    return 1;

  fail:
    if ( info_ic ) av_close_input_file( info_ic );
    return 0;
}

static void ffmpeg_close( hb_stream_t *d )
{
    av_close_input_file( d->ffmpeg_ic );
    if ( d->ffmpeg_pkt != NULL )
    {
        free( d->ffmpeg_pkt );
        d->ffmpeg_pkt = NULL;
    }
}

static void add_ffmpeg_audio( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;
    AVDictionaryEntry *tag;
    int layout;

    // DTS: work around lack of 6.1 support in libhb
    if( hb_ff_dts_request_5point1( codec ) )
    {
        hb_deep_log( 2, "add_ffmpeg_audio: found DTS-ES 6.1, requesting 5.1 core" );
    }

    // scan will ignore any audio without a bitrate. Since we've already
    // typed the audio in order to determine its codec we set up the audio
    // paramters here.
    layout = hb_ff_layout_xlat( codec->channel_layout, codec->channels );
    if ( !layout )
    {
        // Unsupported layout
        return;
    }
    if ( codec->bit_rate || codec->sample_rate )
    {
        hb_audio_t *audio = calloc( 1, sizeof(*audio) );;

        audio->id = id;
        if ( codec->codec_id == CODEC_ID_AC3 )
        {
            audio->config.in.codec = HB_ACODEC_AC3;
        }
        else if ( codec->codec_id == CODEC_ID_DTS &&
                ( codec->profile == FF_PROFILE_DTS ||
                  codec->profile == FF_PROFILE_DTS_96_24 ) )
        {
            audio->config.in.codec = HB_ACODEC_DCA;
        }
        else
        {
            if ( codec->codec_id == CODEC_ID_DTS &&
               ( codec->profile == FF_PROFILE_DTS_ES ||
                 codec->profile == FF_PROFILE_DTS_HD_MA ||
                 codec->profile == FF_PROFILE_DTS_HD_HRA ) )
            {
                audio->config.in.codec = HB_ACODEC_DCA_HD;
            }
            else if ( codec->codec_id == CODEC_ID_AAC )
            {
                int len = MIN(codec->extradata_size, HB_CONFIG_MAX_SIZE);
                memcpy(audio->priv.config.extradata.bytes, codec->extradata, len);
                audio->priv.config.extradata.length = len;
                audio->config.in.codec = HB_ACODEC_FFAAC;
            }
            else if ( codec->codec_id == CODEC_ID_MP3 )
            {
                audio->config.in.codec = HB_ACODEC_MP3;
            }
            else
            {
                audio->config.in.codec = HB_ACODEC_FFMPEG;
            }
            audio->config.in.codec_param = codec->codec_id;

            int bps = av_get_bits_per_sample(codec->codec_id);
            if( bps && codec->sample_rate && codec->channels )
                audio->config.in.bitrate = bps * codec->sample_rate * codec->channels;
            else if( codec->bit_rate )
                audio->config.in.bitrate = codec->bit_rate;
            else
                audio->config.in.bitrate = 1;
            audio->config.in.samplerate = codec->sample_rate;
            audio->config.in.samples_per_frame = codec->frame_size;
            audio->config.in.channel_layout = layout;
            audio->config.in.channel_map = &hb_smpte_chan_map;
        }

        tag = av_dict_get( st->metadata, "language", NULL, 0 );
        set_audio_description( stream, audio,
            lang_for_code2( tag ? tag->value : "und" ) );

        audio->config.in.track = id;
        hb_list_add( title->list_audio, audio );
    }
}

/*
 * Format:
 *   MkvVobSubtitlePrivateData = ( Line )*
 *   Line = FieldName ':' ' ' FieldValue '\n'
 *   FieldName = [^:]+
 *   FieldValue = [^\n]+
 *
 * The line of interest is:
 *   PaletteLine = "palette" ':' ' ' RRGGBB ( ',' ' ' RRGGBB )*
 *
 * More information on the format at:
 *   http://www.matroska.org/technical/specs/subtitles/images.html
 */
static int ffmpeg_parse_vobsub_extradata_mkv( AVCodecContext *codec, hb_subtitle_t *subtitle )
{
    // lines = (string) codec->extradata;
    char *lines = malloc( codec->extradata_size + 1 );
    if ( lines == NULL )
        return 1;
    memcpy( lines, codec->extradata, codec->extradata_size );
    lines[codec->extradata_size] = '\0';

    uint32_t rgb[16];
    int gotPalette = 0;
    int gotDimensions = 0;

    char *curLine, *curLine_parserData;
    for ( curLine = strtok_r( lines, "\n", &curLine_parserData );
          curLine;
          curLine = strtok_r( NULL, "\n", &curLine_parserData ) )
    {
        if (!gotPalette)
        {
            int numElementsRead = sscanf(curLine, "palette: "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x",
                &rgb[0],  &rgb[1],  &rgb[2],  &rgb[3],
                &rgb[4],  &rgb[5],  &rgb[6],  &rgb[7],
                &rgb[8],  &rgb[9],  &rgb[10], &rgb[11],
                &rgb[12], &rgb[13], &rgb[14], &rgb[15]);

            if (numElementsRead == 16) {
                gotPalette = 1;
            }
        }
        if (!gotDimensions)
        {
            int numElementsRead = sscanf(curLine, "size: %dx%d",
                &subtitle->width, &subtitle->height);

            if (numElementsRead == 2) {
                gotDimensions = 1;
            }
        }
        if (gotPalette && gotDimensions)
            break;
    }

    if (subtitle->width == 0 || subtitle->height == 0)
    {
        subtitle->width = 720;
        subtitle->height = 480;
    }

    free( lines );

    if ( gotPalette )
    {
        int i;
        for (i=0; i<16; i++)
            subtitle->palette[i] = hb_rgb2yuv(rgb[i]);
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
 * Format: 8-bit {0,Y,Cb,Cr} x 16
 */
static int ffmpeg_parse_vobsub_extradata_mp4( AVCodecContext *codec, hb_subtitle_t *subtitle )
{
    if ( codec->extradata_size != 4*16 )
        return 1;

    int i, j;
    for ( i=0, j=0; i<16; i++, j+=4 )
    {
        subtitle->palette[i] =
            codec->extradata[j+1] << 16 |   // Y
            codec->extradata[j+2] << 8  |   // Cb
            codec->extradata[j+3] << 0;     // Cr
    }
    if (codec->width <= 0 || codec->height <= 0)
    {
        subtitle->width = 720;
        subtitle->height = 480;
    }
    else
    {
        subtitle->width = codec->width;
        subtitle->height = codec->height;
    }
    return 0;
}

/*
 * Parses the 'subtitle->palette' information from the specific VOB subtitle track's private data.
 * Returns 0 if successful or 1 if parsing failed or was incomplete.
 */
static int ffmpeg_parse_vobsub_extradata( AVCodecContext *codec, hb_subtitle_t *subtitle )
{
    // XXX: Better if we actually chose the correct parser based on the input container
    return
        ffmpeg_parse_vobsub_extradata_mkv( codec, subtitle ) &&
        ffmpeg_parse_vobsub_extradata_mp4( codec, subtitle );
}

static void add_ffmpeg_subtitle( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;

    hb_subtitle_t *subtitle = calloc( 1, sizeof(*subtitle) );

    subtitle->id = id;

    switch ( codec->codec_id )
    {
        case CODEC_ID_DVD_SUBTITLE:
            subtitle->format = PICTURESUB;
            subtitle->source = VOBSUB;
            subtitle->config.dest = RENDERSUB;  // By default render (burn-in) the VOBSUB.
            if ( ffmpeg_parse_vobsub_extradata( codec, subtitle ) )
                hb_log( "add_ffmpeg_subtitle: malformed extradata for VOB subtitle track; "
                        "subtitle colors likely to be wrong" );
            break;
        case CODEC_ID_TEXT:
            subtitle->format = TEXTSUB;
            subtitle->source = UTF8SUB;
            subtitle->config.dest = PASSTHRUSUB;
            break;
        case CODEC_ID_MOV_TEXT: // TX3G
            subtitle->format = TEXTSUB;
            subtitle->source = TX3GSUB;
            subtitle->config.dest = PASSTHRUSUB;
            break;
        case CODEC_ID_SSA:
            subtitle->format = TEXTSUB;
            subtitle->source = SSASUB;
            subtitle->config.dest = PASSTHRUSUB;
            break;
        default:
            hb_log( "add_ffmpeg_subtitle: unknown subtitle stream type: 0x%x", (int) codec->codec_id );
            free(subtitle);
            return;
    }

    AVDictionaryEntry *tag;
    iso639_lang_t *language;

    tag = av_dict_get( st->metadata, "language", NULL, 0 );
    language = lang_for_code2( tag ? tag->value : "und" );
    strcpy( subtitle->lang, language->eng_name );
    strncpy( subtitle->iso639_2, language->iso639_2, 4 );

    // Copy the extradata for the subtitle track
    subtitle->extradata = malloc( codec->extradata_size );
    memcpy( subtitle->extradata, codec->extradata, codec->extradata_size );
    subtitle->extradata_size = codec->extradata_size;

    subtitle->track = id;
    hb_list_add(title->list_subtitle, subtitle);
}

static char *get_ffmpeg_metadata_value( AVDictionary *m, char *key )
{
    AVDictionaryEntry *tag = NULL;
    while ( (tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)) )
    {
        if ( !strcmp( key, tag->key ) )
        {
            return tag->value;
        }
    }
    return NULL;
}

static void add_ffmpeg_attachment( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;

    enum attachtype type;
    const char *name = get_ffmpeg_metadata_value( st->metadata, "filename" );
    switch ( codec->codec_id )
    {
        case CODEC_ID_TTF:
            // Libav sets codec ID based on mime type of the attachment
            type = FONT_TTF_ATTACH;
            break;
        default:
        {
            int len = strlen( name );
            if( len >= 4 &&
                ( !strcmp( name + len - 4, ".ttc" ) ||
                  !strcmp( name + len - 4, ".TTC" ) ||
                  !strcmp( name + len - 4, ".ttf" ) ||
                  !strcmp( name + len - 4, ".TTF" ) ) )
            {
                // Some attachments don't have the right mime type.
                // So also trigger on file name extension.
                type = FONT_TTF_ATTACH;
                break;
            }
            // Ignore unrecognized attachment type
            return;
        }
    }

    hb_attachment_t *attachment = calloc( 1, sizeof(*attachment) );

    // Copy the attachment name and data
    attachment->type = type;
    attachment->name = strdup( name );
    attachment->data = malloc( codec->extradata_size );
    memcpy( attachment->data, codec->extradata, codec->extradata_size );
    attachment->size = codec->extradata_size;

    hb_list_add(title->list_attachment, attachment);
}

static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream, hb_title_t *title )
{
    AVFormatContext *ic = stream->ffmpeg_ic;

    // 'Barebones Title'
    title->type = HB_FF_STREAM_TYPE;
    title->index = 1;

    // Copy part of the stream path to the title name
    char *sep = strrchr(stream->path, '/');
    if (sep)
        strcpy(title->name, sep+1);
    char *dot_term = strrchr(title->name, '.');
    if (dot_term)
        *dot_term = '\0';

    uint64_t dur = ic->duration * 90000 / AV_TIME_BASE;
    title->duration = dur;
    dur /= 90000;
    title->hours    = dur / 3600;
    title->minutes  = ( dur % 3600 ) / 60;
    title->seconds  = dur % 60;

    // set the title to decode the first video stream in the file
    title->demuxer = HB_NULL_DEMUXER;
    title->video_codec = 0;
    int i;
    for (i = 0; i < ic->nb_streams; ++i )
    {
        if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
             avcodec_find_decoder( ic->streams[i]->codec->codec_id ) &&
             title->video_codec == 0 )
        {
            AVCodecContext *context = ic->streams[i]->codec;
            if ( context->pix_fmt != PIX_FMT_YUV420P &&
                 !sws_isSupportedInput( context->pix_fmt ) )
            {
                hb_log( "ffmpeg_title_scan: Unsupported color space" );
                continue;
            }
            title->video_id = i;
            stream->ffmpeg_video_id = i;
            if ( ic->streams[i]->sample_aspect_ratio.num &&
                 ic->streams[i]->sample_aspect_ratio.den )
            {
                title->pixel_aspect_width = ic->streams[i]->sample_aspect_ratio.num;
                title->pixel_aspect_height = ic->streams[i]->sample_aspect_ratio.den;
            }

            if ( context->codec_id == CODEC_ID_H264 )
                title->flags |= HBTF_NO_IDR;

            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = context->codec_id;
        }
        else if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                  avcodec_find_decoder( ic->streams[i]->codec->codec_id ) )
        {
            add_ffmpeg_audio( title, stream, i );
        }
        else if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE )
        {
            add_ffmpeg_subtitle( title, stream, i );
        }
        else if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_ATTACHMENT )
        {
            add_ffmpeg_attachment( title, stream, i );
        }
    }

    title->container_name = strdup( ic->iformat->name );
    title->data_rate = ic->bit_rate;

    hb_deep_log( 2, "Found ffmpeg %d chapters, container=%s", ic->nb_chapters, ic->iformat->name );

    if( ic->nb_chapters != 0 )
    {
        AVChapter *m;
        uint64_t duration_sum = 0;
        for( i = 0; i < ic->nb_chapters; i++ )
            if( ( m = ic->chapters[i] ) != NULL )
            {
                AVDictionaryEntry *tag;
                hb_chapter_t * chapter;
                chapter = calloc( sizeof( hb_chapter_t ), 1 );
                chapter->index    = i+1;
                chapter->duration = ( m->end / ( (double) m->time_base.num * m->time_base.den ) ) * 90000  - duration_sum;
                duration_sum     += chapter->duration;
                chapter->hours    = chapter->duration / 90000 / 3600;
                chapter->minutes  = ( ( chapter->duration / 90000 ) % 3600 ) / 60;
                chapter->seconds  = ( chapter->duration / 90000 ) % 60;
                tag = av_dict_get( m->metadata, "title", NULL, 0 );
                strcpy( chapter->title, tag ? tag->value : "" );
                hb_deep_log( 2, "Added chapter %i, name='%s', dur=%"PRIu64", (%02i:%02i:%02i)",
                            chapter->index, chapter->title,
                            chapter->duration, chapter->hours,
                            chapter->minutes, chapter->seconds );
                hb_list_add( title->list_chapter, chapter );
            }
    }

    /*
     * Fill the metadata.
     */
    decmetadata( title );

    if( hb_list_count( title->list_chapter ) == 0 )
    {
        // Need at least one chapter
        hb_chapter_t * chapter;
        chapter = calloc( sizeof( hb_chapter_t ), 1 );
        chapter->index = 1;
        chapter->duration = title->duration;
        chapter->hours = title->hours;
        chapter->minutes = title->minutes;
        chapter->seconds = title->seconds;
        hb_list_add( title->list_chapter, chapter );
    }

    return title;
}

static int64_t av_to_hb_pts( int64_t pts, double conv_factor )
{
    if ( pts == AV_NOPTS_VALUE )
        return -1;
    return (int64_t)( (double)pts * conv_factor );
}

static int ffmpeg_is_keyframe( hb_stream_t *stream )
{
    uint8_t *pkt;

    switch ( stream->ffmpeg_ic->streams[stream->ffmpeg_video_id]->codec->codec_id )
    {
        case CODEC_ID_VC1:
            // XXX the VC1 codec doesn't mark key frames so to get previews
            // we do it ourselves here. The decoder gets messed up if it
            // doesn't get a SEQ header first so we consider that to be a key frame.
            pkt = stream->ffmpeg_pkt->data;
            if ( !pkt[0] && !pkt[1] && pkt[2] == 1 && pkt[3] == 0x0f )
                return 1;

            return 0;

        case CODEC_ID_WMV3:
            // XXX the ffmpeg WMV3 codec doesn't mark key frames.
            // Only M$ could make I-frame detection this complicated: there
            // are two to four bits of unused junk ahead of the frame type
            // so we have to look at the sequence header to find out how much
            // to skip. Then there are three different ways of coding the type
            // depending on whether it's main or advanced profile then whether
            // there are bframes or not so we have to look at the sequence
            // header to get that.
            pkt = stream->ffmpeg_pkt->data;
            uint8_t *seqhdr = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id]->codec->extradata;
            int pshift = 2;
            if ( ( seqhdr[3] & 0x02 ) == 0 )
                // no FINTERPFLAG
                ++pshift;
            if ( ( seqhdr[3] & 0x80 ) == 0 )
                // no RANGEREDUCTION
                ++pshift;
            if ( seqhdr[3] & 0x70 )
                // stream has b-frames
                return ( ( pkt[0] >> pshift ) & 0x3 ) == 0x01;

            return ( ( pkt[0] >> pshift ) & 0x2 ) == 0;

        default:
            break;
    }
    return ( stream->ffmpeg_pkt->flags & AV_PKT_FLAG_KEY );
}

hb_buffer_t * hb_ffmpeg_read( hb_stream_t *stream )
{
    int err;
    hb_buffer_t * buf;

  again:
    if ( ( err = av_read_frame( stream->ffmpeg_ic, stream->ffmpeg_pkt )) < 0 )
    {
        // av_read_frame can return EAGAIN.  In this case, it expects
        // to be called again to get more data.
        if ( err == AVERROR(EAGAIN) )
        {
            goto again;
        }
        // XXX the following conditional is to handle avi files that
        // use M$ 'packed b-frames' and occasionally have negative
        // sizes for the null frames these require.
        if ( err != AVERROR(ENOMEM) || stream->ffmpeg_pkt->size >= 0 )
            // eof
            return NULL;
    }
    if ( stream->ffmpeg_pkt->stream_index == stream->ffmpeg_video_id )
    {
        if ( stream->need_keyframe )
        {
            // we've just done a seek (generally for scan or live preview) and
            // want to start at a keyframe. Some ffmpeg codecs seek to a key
            // frame but most don't. So we spin until we either get a keyframe
            // or we've looked through 50 video frames without finding one.
            if ( ! ffmpeg_is_keyframe( stream ) && ++stream->need_keyframe < 50 )
            {
                av_free_packet( stream->ffmpeg_pkt );
                goto again;
            }
            stream->need_keyframe = 0;
        }
        ++stream->frames;
    }
    if ( stream->ffmpeg_pkt->size <= 0 )
    {
        // M$ "invalid and inefficient" packed b-frames require 'null frames'
        // following them to preserve the timing (since the packing puts two
        // or more frames in what looks like one avi frame). The contents and
        // size of these null frames are ignored by the ff_h263_decode_frame
        // as long as they're < 20 bytes. We need a positive size so we use
        // one byte if we're given a zero or negative size. We don't know
        // if the pkt data points anywhere reasonable so we just stick a
        // byte of zero in our outbound buf.
        buf = hb_buffer_init( 1 );
        *buf->data = 0;
    }
    else
    {
        // sometimes we get absurd sizes from ffmpeg
        if ( stream->ffmpeg_pkt->size >= (1 << 25) )
        {
            hb_log( "ffmpeg_read: pkt too big: %d bytes", stream->ffmpeg_pkt->size );
            av_free_packet( stream->ffmpeg_pkt );
            return hb_ffmpeg_read( stream );
        }
        buf = hb_buffer_init( stream->ffmpeg_pkt->size );
        memcpy( buf->data, stream->ffmpeg_pkt->data, stream->ffmpeg_pkt->size );
    }
    buf->id = stream->ffmpeg_pkt->stream_index;

    // compute a conversion factor to go from the ffmpeg
    // timebase for the stream to HB's 90kHz timebase.
    AVStream *s = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index];
    double tsconv = 90000. * (double)s->time_base.num / (double)s->time_base.den;

    buf->start = av_to_hb_pts( stream->ffmpeg_pkt->pts, tsconv );
    buf->renderOffset = av_to_hb_pts( stream->ffmpeg_pkt->dts, tsconv );
    if ( buf->renderOffset >= 0 && buf->start == -1 )
    {
        buf->start = buf->renderOffset;
    }
    else if ( buf->renderOffset == -1 && buf->start >= 0 )
    {
        buf->renderOffset = buf->start;
    }

    /*
     * Fill out buf->stop for subtitle packets
     *
     * libavcodec's MKV demuxer stores the duration of UTF-8 subtitles (CODEC_ID_TEXT)
     * in the 'convergence_duration' field for some reason.
     *
     * Other subtitles' durations are stored in the 'duration' field.
     *
     * VOB subtitles (CODEC_ID_DVD_SUBTITLE) do not have their duration stored in
     * either field. This is not a problem because the VOB decoder can extract this
     * information from the packet payload itself.
     *
     * SSA subtitles (CODEC_ID_SSA) do not have their duration stored in
     * either field. This is not a problem because the SSA decoder can extract this
     * information from the packet payload itself.
     */
    enum CodecID ffmpeg_pkt_codec;
    enum AVMediaType codec_type;
    ffmpeg_pkt_codec = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index]->codec->codec_id;
    codec_type = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index]->codec->codec_type;
    switch ( codec_type )
    {
        case AVMEDIA_TYPE_VIDEO:
            buf->type = VIDEO_BUF;
            break;

        case AVMEDIA_TYPE_AUDIO:
            buf->type = AUDIO_BUF;
            break;

        case AVMEDIA_TYPE_SUBTITLE:
            buf->type = SUBTITLE_BUF;
            break;

        default:
            buf->type = OTHER_BUF;
            break;
    }
    if ( ffmpeg_pkt_codec == CODEC_ID_TEXT ) {
        int64_t ffmpeg_pkt_duration = stream->ffmpeg_pkt->convergence_duration;
        int64_t buf_duration = av_to_hb_pts( ffmpeg_pkt_duration, tsconv );
        buf->stop = buf->start + buf_duration;
    }
    if ( ffmpeg_pkt_codec == CODEC_ID_MOV_TEXT ) {
        int64_t ffmpeg_pkt_duration = stream->ffmpeg_pkt->duration;
        int64_t buf_duration = av_to_hb_pts( ffmpeg_pkt_duration, tsconv );
        buf->stop = buf->start + buf_duration;
    }

    /*
     * Check to see whether this buffer is on a chapter
     * boundary, if so mark it as such in the buffer then advance
     * chapter_end to the end of the next chapter.
     * If there are no chapters, chapter_end is always initialized to INT64_MAX
     * (roughly 3 million years at our 90KHz clock rate) so the test
     * below handles both the chapters & no chapters case.
     */
    if ( stream->ffmpeg_pkt->stream_index == stream->ffmpeg_video_id &&
         buf->start >= stream->chapter_end )
    {
        hb_chapter_t *chapter = hb_list_item( stream->title->list_chapter,
                                              stream->chapter+1 );
        if( chapter )
        {
            stream->chapter++;
            stream->chapter_end += chapter->duration;
            buf->new_chap = stream->chapter + 1;
            hb_deep_log( 2, "ffmpeg_read starting chapter %i at %"PRId64,
                         buf->new_chap, buf->start);
        } else {
            // Must have run out of chapters, stop looking.
            stream->chapter_end = INT64_MAX;
        }
    } else {
        buf->new_chap = 0;
    }
    av_free_packet( stream->ffmpeg_pkt );
    return buf;
}

static int ffmpeg_seek( hb_stream_t *stream, float frac )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    if ( frac > 0. )
    {
        int64_t pos = (double)stream->ffmpeg_ic->duration * (double)frac +
                ffmpeg_initial_timestamp( stream );
        avformat_seek_file( ic, -1, 0, pos, pos, AVSEEK_FLAG_BACKWARD);
    }
    else
    {
        int64_t pos = ffmpeg_initial_timestamp( stream );
        avformat_seek_file( ic, -1, 0, pos, pos, AVSEEK_FLAG_BACKWARD);
    }
    stream->need_keyframe = 1;
    return 1;
}

// Assumes that we are always seeking forward
static int ffmpeg_seek_ts( hb_stream_t *stream, int64_t ts )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    int64_t pos;
    int ret;

    pos = ts * AV_TIME_BASE / 90000 + ffmpeg_initial_timestamp( stream );
    AVStream *st = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id];
    // timebase must be adjusted to match timebase of stream we are
    // using for seeking.
    pos = av_rescale(pos, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num);
    stream->need_keyframe = 1;
    // Seek to the nearest timestamp before that requested where
    // there is an I-frame
    ret = avformat_seek_file( ic, stream->ffmpeg_video_id, 0, pos, pos, 0);
    return ret;
}
