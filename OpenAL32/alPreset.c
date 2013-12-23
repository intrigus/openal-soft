
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "alMain.h"
#include "alMidi.h"
#include "alError.h"
#include "alThunk.h"

#include "midi/base.h"


extern inline struct ALsfpreset *LookupPreset(ALCdevice *device, ALuint id);
extern inline struct ALsfpreset *RemovePreset(ALCdevice *device, ALuint id);


AL_API void AL_APIENTRY alGenPresetsSOFT(ALsizei n, ALuint *ids)
{
    ALCdevice *device;
    ALCcontext *context;
    ALsizei cur = 0;
    ALenum err;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);

    device = context->Device;
    for(cur = 0;cur < n;cur++)
    {
        ALsfpreset *preset = calloc(1, sizeof(ALsfpreset));
        if(!preset)
        {
            alDeletePresetsSOFT(cur, ids);
            SET_ERROR_AND_GOTO(context, AL_OUT_OF_MEMORY, done);
        }
        ALsfpreset_Construct(preset);

        err = NewThunkEntry(&preset->id);
        if(err == AL_NO_ERROR)
            err = InsertUIntMapEntry(&device->PresetMap, preset->id, preset);
        if(err != AL_NO_ERROR)
        {
            ALsfpreset_Destruct(preset);
            memset(preset, 0, sizeof(*preset));
            free(preset);

            alDeletePresetsSOFT(cur, ids);
            SET_ERROR_AND_GOTO(context, err, done);
        }

        ids[cur] = preset->id;
    }

done:
    ALCcontext_DecRef(context);
}

AL_API ALvoid AL_APIENTRY alDeletePresetsSOFT(ALsizei n, const ALuint *ids)
{
    ALCdevice *device;
    ALCcontext *context;
    ALsfpreset *preset;
    ALsizei i;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);

    device = context->Device;
    for(i = 0;i < n;i++)
    {
        if(!ids[i])
            continue;

        /* Check for valid ID */
        if((preset=LookupPreset(device, ids[i])) == NULL)
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
        if(preset->ref != 0)
            SET_ERROR_AND_GOTO(context, AL_INVALID_OPERATION, done);
    }

    for(i = 0;i < n;i++)
    {
        if((preset=RemovePreset(device, ids[i])) == NULL)
            continue;

        ALsfpreset_Destruct(preset);

        memset(preset, 0, sizeof(*preset));
        free(preset);
    }

done:
    ALCcontext_DecRef(context);
}

AL_API ALboolean AL_APIENTRY alIsPresetSOFT(ALuint id)
{
    ALCcontext *context;
    ALboolean ret;

    context = GetContextRef();
    if(!context) return AL_FALSE;

    ret = ((!id || LookupPreset(context->Device, id)) ?
           AL_TRUE : AL_FALSE);

    ALCcontext_DecRef(context);

    return ret;
}

AL_API void AL_APIENTRY alPresetiSOFT(ALuint id, ALenum param, ALint value)
{
    ALCdevice *device;
    ALCcontext *context;
    ALsfpreset *preset;

    context = GetContextRef();
    if(!context) return;

    device = context->Device;
    if((preset=LookupPreset(device, id)) == NULL)
        SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    if(preset->ref != 0)
        SET_ERROR_AND_GOTO(context, AL_INVALID_OPERATION, done);
    switch(param)
    {
        case AL_MIDI_PRESET_SOFT:
            if(!(value >= 0 && value <= 127))
                SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
            preset->Preset = value;
            break;

        case AL_MIDI_BANK_SOFT:
            if(!(value >= 0 && value <= 128))
                SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
            preset->Bank = value;
            break;

        default:
            SET_ERROR_AND_GOTO(context, AL_INVALID_ENUM, done);
    }

done:
    ALCcontext_DecRef(context);
}

AL_API void AL_APIENTRY alPresetivSOFT(ALuint id, ALenum param, const ALint *values)
{
    ALCdevice *device;
    ALCcontext *context;
    ALsfpreset *preset;

    switch(param)
    {
        case AL_MIDI_PRESET_SOFT:
        case AL_MIDI_BANK_SOFT:
            alPresetiSOFT(id, param, values[0]);
            return;
    }

    context = GetContextRef();
    if(!context) return;

    device = context->Device;
    if((preset=LookupPreset(device, id)) == NULL)
        SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    if(preset->ref != 0)
        SET_ERROR_AND_GOTO(context, AL_INVALID_OPERATION, done);
    switch(param)
    {
        default:
            SET_ERROR_AND_GOTO(context, AL_INVALID_ENUM, done);
    }

done:
    ALCcontext_DecRef(context);
}

AL_API void AL_APIENTRY alGetPresetivSOFT(ALuint id, ALenum param, ALint *values)
{
    ALCdevice *device;
    ALCcontext *context;
    ALsfpreset *preset;

    context = GetContextRef();
    if(!context) return;

    device = context->Device;
    if((preset=LookupPreset(device, id)) == NULL)
        SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    if(preset->ref != 0)
        SET_ERROR_AND_GOTO(context, AL_INVALID_OPERATION, done);
    switch(param)
    {
        case AL_MIDI_PRESET_SOFT:
            values[0] = preset->Preset;
            break;

        case AL_MIDI_BANK_SOFT:
            values[0] = preset->Bank;
            break;

        default:
            SET_ERROR_AND_GOTO(context, AL_INVALID_ENUM, done);
    }

done:
    ALCcontext_DecRef(context);
}


/* ReleaseALPresets
 *
 * Called to destroy any presets that still exist on the device
 */
void ReleaseALPresets(ALCdevice *device)
{
    ALsizei i;
    for(i = 0;i < device->PresetMap.size;i++)
    {
        ALsfpreset *temp = device->PresetMap.array[i].value;
        device->PresetMap.array[i].value = NULL;

        ALsfpreset_Destruct(temp);

        memset(temp, 0, sizeof(*temp));
        free(temp);
    }
}
