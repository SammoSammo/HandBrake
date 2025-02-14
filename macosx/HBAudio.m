//
//  HBAudio.m
//  HandBrake
//
//  Created on 2010-08-30.
//

#import "HBAudio.h"
#import "HBAudioController.h"
#import "hb.h"

NSString *keyAudioCodecName = @"keyAudioCodecName";
NSString *keyAudioMP4 = @"keyAudioMP4";
NSString *keyAudioMKV = @"keyAudioMKV";
NSString *keyAudioSampleRateName = @"keyAudioSampleRateName";
NSString *keyAudioBitrateName = @"keyAudioBitrateName";
NSString *keyAudioMustMatchTrack = @"keyAudioMustMatchTrack";
NSString *keyAudioMixdownName = @"keyAudioMixdownName";

NSString *keyAudioCodec = @"codec";
NSString *keyAudioMixdown = @"mixdown";
NSString *keyAudioSamplerate = @"samplerate";
NSString *keyAudioBitrate = @"bitrate";

static NSMutableArray *masterCodecArray = nil;
static NSMutableArray *masterMixdownArray = nil;
static NSMutableArray *masterSampleRateArray = nil;
static NSMutableArray *masterBitRateArray = nil;

@interface NSArray (HBAudioSupport)
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey;
- (NSDictionary *) lastDictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey;
@end
@implementation NSArray (HBAudioSupport)
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey reverse: (BOOL) reverse

{
    NSDictionary *retval = nil;
    NSEnumerator *enumerator = reverse ? [self reverseObjectEnumerator] : [self objectEnumerator];
    NSDictionary *dict;
    id aValue;

    while (nil != (dict = [enumerator nextObject]) && !retval)
    {
        if (nil != (aValue = [dict objectForKey: aKey]) && [aValue isEqual: anObject])
        {
            retval = dict;
        }
    }
    return retval;
}
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey
{
    return [self dictionaryWithObject: anObject matchingKey: aKey reverse: NO];
}
- (NSDictionary *) lastDictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey
{
    return [self dictionaryWithObject: anObject matchingKey: aKey reverse: YES];
}

@end

@implementation HBAudio

#pragma mark -
#pragma mark Object Setup

+ (void) load

{
    if ([HBAudio class] == self)
    {
        int i, audioMustMatch, shouldAdd;
        BOOL muxMKV, muxMP4;
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        NSDictionary *dict;

        masterCodecArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for( i = 0; i < hb_audio_encoders_count; i++ )
        {
            if( ( hb_audio_encoders[i].encoder & HB_ACODEC_PASS_FLAG ) &&
                ( hb_audio_encoders[i].encoder != HB_ACODEC_AUTO_PASS ) )
            {
                audioMustMatch = ( hb_audio_encoders[i].encoder & ~HB_ACODEC_PASS_FLAG );
            }
            else
            {
                audioMustMatch = 0;
            }
            // Auto Passthru disabled until we have GUI widgets for it
            shouldAdd = ( hb_audio_encoders[i].encoder != HB_ACODEC_AUTO_PASS ) &&
                        ( ( hb_audio_encoders[i].encoder != HB_ACODEC_CA_HAAC ) || encca_haac_available() );
            muxMKV = ( hb_audio_encoders[i].muxers & HB_MUX_MKV ) ? YES : NO;
            muxMP4 = ( hb_audio_encoders[i].muxers & HB_MUX_MP4 ) ? YES : NO;
            if( shouldAdd && audioMustMatch )
            {
                [masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
                                              [NSString stringWithUTF8String: hb_audio_encoders[i].human_readable_name], keyAudioCodecName,
                                              [NSNumber numberWithInt: hb_audio_encoders[i].encoder], keyAudioCodec,
                                              [NSNumber numberWithBool: muxMP4], keyAudioMP4,
                                              [NSNumber numberWithBool: muxMKV], keyAudioMKV,
                                              [NSNumber numberWithInt: audioMustMatch], keyAudioMustMatchTrack,
                                              nil]];
            }
            else if( shouldAdd )
            {
                [masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
                                              [NSString stringWithUTF8String: hb_audio_encoders[i].human_readable_name], keyAudioCodecName,
                                              [NSNumber numberWithInt: hb_audio_encoders[i].encoder], keyAudioCodec,
                                              [NSNumber numberWithBool: muxMP4], keyAudioMP4,
                                              [NSNumber numberWithBool: muxMKV], keyAudioMKV,
                                              [NSNumber numberWithBool: NO], keyAudioMustMatchTrack,
                                              nil]];
            }
        }

        masterMixdownArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (i = 0; i < hb_audio_mixdowns_count; i++)
        {
            [masterMixdownArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
                                            [NSString stringWithUTF8String: hb_audio_mixdowns[i].human_readable_name], keyAudioMixdownName,
                                            [NSNumber numberWithInt: hb_audio_mixdowns[i].amixdown], keyAudioMixdown,
                                            nil]];
        }

        // Note that for the Auto value we use 0 for the sample rate because our controller will give back the track's
        // input sample rate when it finds this 0 value as the selected sample rate.  We do this because the input
        // sample rate depends on the track, which means it depends on the title, so cannot be nicely set up here.
        masterSampleRateArray = [[NSMutableArray alloc] init]; // knowingly leaked
        [masterSampleRateArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
                                           NSLocalizedString(@"Auto", @"Auto"), keyAudioSampleRateName,
                                           [NSNumber numberWithInt: 0], keyAudioSamplerate,
                                           nil]];
        for (i = 0; i < hb_audio_rates_count; i++)
        {
            [masterSampleRateArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
                                               [NSString stringWithUTF8String: hb_audio_rates[i].string], keyAudioSampleRateName,
                                               [NSNumber numberWithInt: hb_audio_rates[i].rate], keyAudioSamplerate,
                                               nil]];
        }

        masterBitRateArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (i = 0; i < hb_audio_bitrates_count; i++)
        {
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithUTF8String: hb_audio_bitrates[i].string], keyAudioBitrateName,
                    [NSNumber numberWithInt: hb_audio_bitrates[i].rate], keyAudioBitrate,
                    nil];
            [masterBitRateArray addObject: dict];
        }

        [pool release];
    }
}

// Ensure the list of codecs is accurate
// Update the current value of codec based on the revised list
- (void) updateCodecs

{
    NSMutableArray *permittedCodecs = [NSMutableArray array];
    unsigned int count = [masterCodecArray count];
    NSDictionary *dict;
    NSString *keyThatAllows = nil;

    // Determine which key we use to see which codecs are permitted
    switch ([videoContainerTag intValue])
    {
        case HB_MUX_MP4:
            keyThatAllows = keyAudioMP4;
            break;
        case HB_MUX_MKV:
            keyThatAllows = keyAudioMKV;
            break;
        default:
            keyThatAllows = @"error condition";
            break;
    }

    // First get a list of the permitted codecs based on the internal rules
    if (nil != track && [self enabled])
    {
        BOOL goodToAdd;

        for (unsigned int i = 0; i < count; i++)
        {
            dict = [masterCodecArray objectAtIndex: i];

            // First make sure only codecs permitted by the container are here
            goodToAdd = [[dict objectForKey: keyThatAllows] boolValue];

            // Now we make sure if DTS or AC3 is not available in the track it is not put in the codec list, but in a general way
            if ([[dict objectForKey: keyAudioMustMatchTrack] boolValue])
            {
                if ([[dict objectForKey: keyAudioMustMatchTrack] intValue] != [[[self track] objectForKey: keyAudioInputCodec] intValue])
                {
                    goodToAdd = NO;
                }
            }

            if (goodToAdd)
            {
                [permittedCodecs addObject: dict];
            }
        }
    }

    // Now make sure the permitted list and the actual ones matches
    [self setCodecs: permittedCodecs];

    // Ensure our codec is on the list of permitted codecs
    if (![self codec] || ![permittedCodecs containsObject: [self codec]])
    {
        if (0 < [permittedCodecs count])
        {
            [self setCodec: [permittedCodecs objectAtIndex: 0]]; // This should be defaulting to Core Audio
        }
        else
        {
            [self setCodec: nil];
        }
    }
}

- (void) updateMixdowns: (BOOL) shouldSetDefault

{
    NSMutableArray *permittedMixdowns = [NSMutableArray array];
    NSDictionary *dict;
    BOOL shouldAdd;
    int currentMixdown;

    unsigned int count = [masterMixdownArray count];
    int codecCodec = [[codec objectForKey: keyAudioCodec] intValue];
    int channelLayout = [[track objectForKey: keyAudioInputChannelLayout] intValue];
    int theDefaultMixdown = hb_get_default_mixdown(codecCodec, channelLayout);
    int theBestMixdown = hb_get_best_mixdown(codecCodec, channelLayout, 0);

    for (unsigned int i = 0; i < count; i++)
    {
        dict = [masterMixdownArray objectAtIndex: i];
        currentMixdown = [[dict objectForKey: keyAudioMixdown] intValue];

        // Basically with the way the mixdowns are stored, the assumption from the libhb point of view
        // currently is that all mixdowns from the best down to mono are supported.
        if ((currentMixdown != HB_AMIXDOWN_NONE) && (currentMixdown <= theBestMixdown))
        {
            shouldAdd = YES;
        }
        else if ((currentMixdown == HB_AMIXDOWN_NONE) && (codecCodec & HB_ACODEC_PASS_FLAG))
        {
            // "None" mixdown (passthru)
            shouldAdd = YES;
        }
        else
        {
            shouldAdd = NO;
        }

        if (shouldAdd)
        {
            [permittedMixdowns addObject: dict];
        }
    }

    if (![self enabled])
    {
        permittedMixdowns = nil;
    }

    // Now make sure the permitted list and the actual ones matches
    [self setMixdowns: permittedMixdowns];

    // Select the proper one
    if (shouldSetDefault)
    {
        [self setMixdown: [permittedMixdowns dictionaryWithObject: [NSNumber numberWithInt: theDefaultMixdown]
                                                      matchingKey: keyAudioMixdown]];
    }

    if (![self mixdown] || ![permittedMixdowns containsObject: [self mixdown]])
    {
        [self setMixdown: [permittedMixdowns lastObject]];
    }
}

- (void) updateBitRates: (BOOL) shouldSetDefault

{
    NSMutableArray *permittedBitRates = [NSMutableArray array];
    NSDictionary *dict;
    int minBitRate;
    int maxBitRate;
    int currentBitRate;
    BOOL shouldAdd;

    unsigned int count = [masterBitRateArray count];
    int trackInputBitRate = [[[self track] objectForKey: keyAudioInputBitrate] intValue];
    int theSampleRate = [[[self sampleRate] objectForKey: keyAudioSamplerate] intValue];

    if (0 == theSampleRate) // this means Auto
    {
        theSampleRate = [[[self track] objectForKey: keyAudioInputSampleRate] intValue];
    }

    int ourCodec = [[codec objectForKey: keyAudioCodec] intValue];
    int ourMixdown = [[[self mixdown] objectForKey: keyAudioMixdown] intValue];
    hb_get_audio_bitrate_limits(ourCodec, theSampleRate, ourMixdown, &minBitRate, &maxBitRate);
    int theDefaultBitRate = hb_get_default_audio_bitrate(ourCodec, theSampleRate, ourMixdown);

    BOOL codecIsPassthru = ([[codec objectForKey: keyAudioCodec] intValue] & HB_ACODEC_PASS_FLAG) ? YES : NO;
    BOOL codecIsLossless = (theDefaultBitRate == -1) ? YES : NO;

    if (codecIsPassthru)
    {
        NSDictionary *sourceBitRate = [masterBitRateArray dictionaryWithObject: [NSNumber numberWithInt: trackInputBitRate]
                                                                   matchingKey: keyAudioBitrate];
        if (!sourceBitRate)
        {
            // the source bitrate isn't in the master array - create it
            sourceBitRate = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSString stringWithFormat: @"%d", trackInputBitRate], keyAudioBitrateName,
                             [NSNumber numberWithInt: trackInputBitRate], keyAudioBitrate,
                             nil];
        }
        [permittedBitRates addObject: sourceBitRate];
    }
    else if (codecIsLossless)
    {
        NSDictionary *bitRateNotApplicable = [NSDictionary dictionaryWithObjectsAndKeys:
                                              [NSString stringWithString: @"N/A"], keyAudioBitrateName,
                                              [NSNumber numberWithInt: -1], keyAudioBitrate,
                                              nil];
        [permittedBitRates addObject: bitRateNotApplicable];
    }
    else
    {
        for (unsigned int i = 0; i < count; i++)
        {
            dict = [masterBitRateArray objectAtIndex: i];
            currentBitRate = [[dict objectForKey: keyAudioBitrate] intValue];

            // First ensure the bitrate falls within range of the codec
            shouldAdd = (currentBitRate >= minBitRate && currentBitRate <= maxBitRate);

            if (shouldAdd)
            {
                [permittedBitRates addObject: dict];
            }
        }
    }

    if (![self enabled])
    {
        permittedBitRates = nil;
    }

    // Make sure we are updated with the permitted list
    [self setBitRates: permittedBitRates];

    // Select the proper one
    if (shouldSetDefault)
    {
        [self setBitRateFromName: [NSString stringWithFormat:@"%d", theDefaultBitRate]];
    }

    if (![self bitRate] || ![permittedBitRates containsObject: [self bitRate]])
    {
        [self setBitRate: [permittedBitRates lastObject]];
    }
}

- (id) init

{
    if (self = [super init])
    {
        [self addObserver: self forKeyPath: @"videoContainerTag" options: 0 context: NULL];
        [self addObserver: self forKeyPath: @"track" options: NSKeyValueObservingOptionOld context: NULL];
        [self addObserver: self forKeyPath: @"codec" options: 0 context: NULL];
        [self addObserver: self forKeyPath: @"mixdown" options: 0 context: NULL];
        [self addObserver: self forKeyPath: @"sampleRate" options: 0 context: NULL];
    }
    return self;
}

#pragma mark -
#pragma mark Accessors

@synthesize track;
@synthesize codec;
@synthesize mixdown;
@synthesize sampleRate;
@synthesize bitRate;
@synthesize drc;
@synthesize gain;
@synthesize videoContainerTag;
@synthesize controller;

@synthesize codecs;
@synthesize mixdowns;
@synthesize bitRates;

- (NSArray *) sampleRates
{
    return masterSampleRateArray;
}

- (void) dealloc

{
    [self removeObserver: self forKeyPath: @"videoContainerTag"];
    [self removeObserver: self forKeyPath: @"track"];
    [self removeObserver: self forKeyPath: @"codec"];
    [self removeObserver: self forKeyPath: @"mixdown"];
    [self removeObserver: self forKeyPath: @"sampleRate"];
    [self setTrack: nil];
    [self setCodec: nil];
    [self setMixdown: nil];
    [self setSampleRate: nil];
    [self setBitRate: nil];
    [self setDrc: nil];
    [self setGain: nil];
    [self setVideoContainerTag: nil];
    [self setCodecs: nil];
    [self setMixdowns: nil];
    [self setBitRates: nil];
    [super dealloc];
}

#pragma mark -
#pragma mark KVO

- (void) observeValueForKeyPath: (NSString *) keyPath ofObject: (id) object change: (NSDictionary *) change context: (void *) context

{
    if ([keyPath isEqualToString: @"videoContainerTag"])
    {
        [self updateCodecs];
    }
    else if ([keyPath isEqualToString: @"track"])
    {
        if (nil != [self track])
        {
            [self updateCodecs];
            [self updateMixdowns: YES];
            if ([self enabled])
            {
                [self setSampleRate: [[self sampleRates] objectAtIndex: 0]]; // default to Auto
            }
            if ([[controller noneTrack] isEqual: [change objectForKey: NSKeyValueChangeOldKey]])
            {
                [controller switchingTrackFromNone: self];
            }
            if ([[controller noneTrack] isEqual: [self track]])
            {
                [controller settingTrackToNone: self];
            }
        }
    }
    else if ([keyPath isEqualToString: @"codec"])
    {
        [self updateMixdowns: YES];
        [self updateBitRates: YES];
    }
    else if ([keyPath isEqualToString: @"mixdown"])
    {
        [self updateBitRates: YES];
        [[NSNotificationCenter defaultCenter] postNotificationName: HBMixdownChangedNotification object: self];
    }
    else if ([keyPath isEqualToString: @"sampleRate"])
    {
        [self updateBitRates: NO];
    }
}

#pragma mark -
#pragma mark Special Setters

- (void) setTrackFromIndex: (int) aValue

{
    [self setTrack: [self.controller.masterTrackArray dictionaryWithObject: [NSNumber numberWithInt: aValue]
                                                               matchingKey: keyAudioTrackIndex]];
}

// This returns whether it is able to set the actual codec desired.
- (BOOL) setCodecFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self codecs] dictionaryWithObject: aValue matchingKey: keyAudioCodecName];

    if (nil != dict)
    {
        [self setCodec: dict];
    }
    return (nil != dict);
}

- (void) setMixdownFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self mixdowns] dictionaryWithObject: aValue matchingKey: keyAudioMixdownName];

    if (nil != dict)
    {
        [self setMixdown: dict];
    }
}

- (void) setSampleRateFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self sampleRates] dictionaryWithObject: aValue matchingKey: keyAudioSampleRateName];

    if (nil != dict)
    {
        [self setSampleRate: dict];
    }
}

- (void) setBitRateFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self bitRates] dictionaryWithObject: aValue matchingKey: keyAudioBitrateName];

    if (nil != dict)
    {
        [self setBitRate: dict];
    }
}


#pragma mark -
#pragma mark Validation

// Because we have indicated that the binding for the drc validates immediately we can implement the
// key value binding method to ensure the drc stays in our accepted range.
- (BOOL) validateDrc: (id *) ioValue error: (NSError *) outError

{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        if (0.0 < [*ioValue floatValue] && 1.0 > [*ioValue floatValue])
        {
            *ioValue = [NSNumber numberWithFloat: 1.0];
        }
    }

    return retval;
}

// Because we have indicated that the binding for the gain validates immediately we can implement the
// key value binding method to ensure the gain stays in our accepted range.

- (BOOL) validateGain: (id *) ioValue error: (NSError *) outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        if (0.0 < [*ioValue floatValue] && 1.0 > [*ioValue floatValue])
        {
            *ioValue = [NSNumber numberWithFloat: 0.0];
        }
    }

    return retval;
}

#pragma mark -
#pragma mark Bindings Support

- (BOOL) enabled

{
    return (nil != track) ? (![track isEqual: [controller noneTrack]]) : NO;
}

- (BOOL) mixdownEnabled

{
    BOOL retval = [self enabled];

    if (retval)
    {
        int myMixdown = [[[self mixdown] objectForKey: keyAudioMixdown] intValue];
        if (myMixdown == HB_AMIXDOWN_NONE)
        {
            // "None" mixdown (passthru)
            retval = NO;
        }
    }
    return retval;
}

- (BOOL) bitrateEnabled

{
    BOOL retval = [self enabled];

    if (retval)
    {
        int myCodecCodec = [[[self codec] objectForKey: keyAudioCodec] intValue];
        int myCodecDefaultBitrate = hb_get_default_audio_bitrate(myCodecCodec, 0, 0);
        if (myCodecDefaultBitrate < 0)
        {
            retval = NO;
        }
    }
    return retval;
}

- (BOOL) AC3Enabled

{
    BOOL retval = [self enabled];

    if (retval)
    {
        int myTrackCodec = [[[self track] objectForKey: keyAudioInputCodec] intValue];
        int myCodecCodec = [[[self codec] objectForKey: keyAudioCodec] intValue];
        if (HB_ACODEC_AC3 != myTrackCodec || HB_ACODEC_AC3_PASS == myCodecCodec)
        {
            retval = NO;
        }
    }
    return retval;
}

- (BOOL) PassThruDisabled

{
    BOOL retval = [self enabled];

    if (retval)
    {
        int myCodecCodec = [[[self codec] objectForKey: keyAudioCodec] intValue];
        if (myCodecCodec & HB_ACODEC_PASS_FLAG)
        {
            retval = NO;
        }
    }
    return retval;
}

+ (NSSet *) keyPathsForValuesAffectingValueForKey: (NSString *) key

{
    NSSet *retval = nil;

    if ([key isEqualToString: @"enabled"])
    {
        retval = [NSSet setWithObjects: @"track", nil];
    }
    else if ([key isEqualToString: @"PassThruDisabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"codec", nil];
    }
    else if ([key isEqualToString: @"AC3Enabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"codec", nil];
    }
    else if ([key isEqualToString: @"bitrateEnabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"codec", nil];
    }
    else if ([key isEqualToString: @"mixdownEnabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"mixdown", nil];
    }
    return retval;
}

@end

