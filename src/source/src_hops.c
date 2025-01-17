   
   /**
    * \file     src_hops.c
    * \author   François Grondin <francois.grondin2@usherbrooke.ca>
    * \version  2.0
    * \date     2018-03-18
    * \copyright
    *
    * This program is free software: you can redistribute it and/or modify
    * it under the terms of the GNU General Public License as published by
    * the Free Software Foundation, either version 3 of the License, or
    * (at your option) any later version.
    *
    * This program is distributed in the hope that it will be useful,
    * but WITHOUT ANY WARRANTY; without even the implied warranty of
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    * GNU General Public License for more details.
    * 
    * You should have received a copy of the GNU General Public License
    * along with this program.  If not, see <http://www.gnu.org/licenses/>.
    *
    */
    
    #include <source/src_hops.h>

    src_hops_obj * src_hops_construct(const src_hops_cfg * src_hops_config, const msg_hops_cfg * msg_hops_config) {

        src_hops_obj * obj;
        unsigned int nBytes;

        obj = (src_hops_obj *) malloc(sizeof(src_hops_obj));

        obj->timeStamp = 0;
        obj->timeStampMap = NULL;

        obj->hopSize = msg_hops_config->hopSize;
        obj->nChannels = msg_hops_config->nChannels;
        obj->fS = msg_hops_config->fS;
        
        obj->format = format_clone(src_hops_config->format);
        obj->interface = interface_clone(src_hops_config->interface);

        memset(obj->bytes, 0x00, 4 * sizeof(char));

        if (!(((obj->interface->type == interface_file)  && (obj->format->type == format_binary_int08)) ||
              ((obj->interface->type == interface_file)  && (obj->format->type == format_binary_int16)) ||
              ((obj->interface->type == interface_file)  && (obj->format->type == format_binary_int24)) ||
              ((obj->interface->type == interface_file)  && (obj->format->type == format_binary_int32)) ||
              ((obj->interface->type == interface_soundcard)  && (obj->format->type == format_binary_int08)) ||
              ((obj->interface->type == interface_soundcard)  && (obj->format->type == format_binary_int16)) ||
              ((obj->interface->type == interface_soundcard)  && (obj->format->type == format_binary_int24)) ||
              ((obj->interface->type == interface_soundcard)  && (obj->format->type == format_binary_int32)) ||
              ((obj->interface->type == interface_pulseaudio)  && (obj->format->type == format_binary_int08)) ||
              ((obj->interface->type == interface_pulseaudio)  && (obj->format->type == format_binary_int16)) ||
              ((obj->interface->type == interface_pulseaudio)  && (obj->format->type == format_binary_int24)) ||
              ((obj->interface->type == interface_pulseaudio)  && (obj->format->type == format_binary_int32)))) {
            
            printf("Source hops: Invalid interface and/or format.\n");
            exit(EXIT_FAILURE);

        }

        obj->buffer = (char *) malloc(sizeof(char) * obj->hopSize * obj->nChannels * 4);
        memset(obj->buffer, 0x00, sizeof(char) * obj->hopSize * obj->nChannels * 4);

        switch (obj->format->type) {

            case format_binary_int08: obj->bufferSize = obj->hopSize * obj->nChannels * 1; break;
            case format_binary_int16: obj->bufferSize = obj->hopSize * obj->nChannels * 2; break;
            case format_binary_int24: obj->bufferSize = obj->hopSize * obj->nChannels * 3; break;
            case format_binary_int32: obj->bufferSize = obj->hopSize * obj->nChannels * 4; break;

        }      

        obj->out = (msg_hops_obj *) NULL;
        
        return obj;

    }

    void src_hops_destroy(src_hops_obj * obj) {

        free((void *) obj->buffer);
        format_destroy(obj->format);
        interface_destroy(obj->interface);

        free((void *) obj);

    }

    void src_hops_connect(src_hops_obj * obj, msg_hops_obj * out) {

        obj->out = out;

    }

    void src_hops_disconnect(src_hops_obj * obj) {

        obj->out = (msg_hops_obj *) NULL;

    }

    void src_hops_open(src_hops_obj * obj) {

        switch(obj->interface->type) {

            case interface_file:

                src_hops_open_interface_file(obj);

            break;

            case interface_soundcard:

                src_hops_open_interface_soundcard(obj);

            break;

            case interface_pulseaudio:

                src_hops_open_interface_pulseaudio(obj);

            break;

            default:

                printf("Source hops: Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;

        }        

    }

    void src_hops_open_interface_file(src_hops_obj * obj) {

        obj->fp = fopen(obj->interface->fileName, "rb");

        if (obj->fp == NULL) {
            printf("Cannot open file %s\n",obj->interface->fileName);
            exit(EXIT_FAILURE);
        }

    }

    void src_hops_open_interface_pulseaudio(src_hops_obj * obj) {

        pa_sample_format_t format;

        switch (obj->format->type) {
            
            case format_binary_int08:

                format = PA_SAMPLE_U8;

            break;

            case format_binary_int16:

                format = PA_SAMPLE_S16LE;

            break;

            case format_binary_int24:

                format = PA_SAMPLE_S24LE;

            break;
            
            case format_binary_int32:
            
                format = PA_SAMPLE_S32LE;
            
            break;

            default:

                printf("Source hops: Invalid format.\n");
                exit(EXIT_FAILURE);

            break;

        }

        obj->ss.format = format;
        obj->ss.rate = obj->fS;
        obj->ss.channels = obj->nChannels;

        obj->map.channels = obj->nChannels;
        switch(obj->nChannels)
        {
            case 1:
                obj->map.map[0] = PA_CHANNEL_POSITION_MONO;
                break;

            case 2:
                obj->map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
                obj->map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
                break;

            case 8:
                obj->map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
                obj->map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
                obj->map.map[2] = PA_CHANNEL_POSITION_REAR_LEFT;
                obj->map.map[3] = PA_CHANNEL_POSITION_REAR_RIGHT;
                obj->map.map[4] = PA_CHANNEL_POSITION_FRONT_CENTER;
                obj->map.map[5] = PA_CHANNEL_POSITION_LFE;
                obj->map.map[6] = PA_CHANNEL_POSITION_SIDE_LEFT;
                obj->map.map[7] = PA_CHANNEL_POSITION_SIDE_RIGHT;
                break;

            case 16:
                obj->map.map[0]  = PA_CHANNEL_POSITION_FRONT_LEFT;
                obj->map.map[1]  = PA_CHANNEL_POSITION_FRONT_RIGHT;
                obj->map.map[2]  = PA_CHANNEL_POSITION_REAR_LEFT;
                obj->map.map[3]  = PA_CHANNEL_POSITION_REAR_RIGHT;
                obj->map.map[4]  = PA_CHANNEL_POSITION_FRONT_CENTER;
                obj->map.map[5]  = PA_CHANNEL_POSITION_LFE;
                obj->map.map[6]  = PA_CHANNEL_POSITION_SIDE_LEFT;
                obj->map.map[7]  = PA_CHANNEL_POSITION_SIDE_RIGHT;
                obj->map.map[8]  = PA_CHANNEL_POSITION_AUX0;
                obj->map.map[9]  = PA_CHANNEL_POSITION_AUX1;
                obj->map.map[10] = PA_CHANNEL_POSITION_AUX2;
                obj->map.map[11] = PA_CHANNEL_POSITION_AUX3;
                obj->map.map[12] = PA_CHANNEL_POSITION_AUX4;
                obj->map.map[13] = PA_CHANNEL_POSITION_AUX5;
                obj->map.map[14] = PA_CHANNEL_POSITION_AUX6;
                obj->map.map[15] = PA_CHANNEL_POSITION_AUX7;
                break;

            default:

                printf("Source hops: Invalid format.\n");
                exit(EXIT_FAILURE);

            break;
        }

        int err;
        if (!(obj->pa = pa_simple_new(NULL, "Odas", PA_STREAM_RECORD, obj->interface->deviceName, "record", &obj->ss, &obj->map, NULL, &err))) {
            printf("Source hops: Cannot open pulseaudio device %s: %s\n", obj->interface->deviceName, pa_strerror(err));
            exit(EXIT_FAILURE);
        }
        
    }

    void src_hops_open_interface_soundcard(src_hops_obj * obj) {

        snd_pcm_hw_params_t * hw_params;
        snd_pcm_format_t format;
        int err;

        switch (obj->format->type) {
            
            case format_binary_int08:

                format = SND_PCM_FORMAT_S8;

            break;

            case format_binary_int16:

                format = SND_PCM_FORMAT_S16_LE;

            break;

            case format_binary_int24:

                format = SND_PCM_FORMAT_S24_LE;

            break;
            
            case format_binary_int32:
            
                format = SND_PCM_FORMAT_S32_LE;
            
            break;

            default:

                printf("Source hops: Invalid format.\n");
                exit(EXIT_FAILURE);

            break;

        }       


        if ((err = snd_pcm_open(&(obj->ch), obj->interface->deviceName, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
            printf("Source hops: Cannot open audio device %s: %s\n",obj->interface->deviceName, snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
            printf("Source hops: Cannot allocate hardware parameter structure: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params_any(obj->ch, hw_params)) < 0) {
            printf("Source hops: Cannot initialize hardware parameter structure: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params_set_access(obj->ch, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            printf("Source hops: Cannot set access type: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params_set_format(obj->ch, hw_params, format)) < 0) {
            printf("Source hops: Cannot set sample format: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params_set_rate(obj->ch, hw_params, obj->fS, 0)) < 0) {
            printf("Source hops: Cannot set sample rate: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params_set_channels(obj->ch, hw_params, obj->nChannels)) < 0) {
            printf("Source hops: Cannot set channel count: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_hw_params(obj->ch, hw_params)) < 0) {
            printf("Source hops: Cannot set parameters: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        snd_pcm_hw_params_free(hw_params);

        if ((err = snd_pcm_prepare(obj->ch)) < 0) {
            printf("Source hops: Cannot prepare audio interface for use: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

    }

    void src_hops_close(src_hops_obj * obj) {

        switch(obj->interface->type) {

            case interface_file:

                src_hops_close_interface_file(obj);

            break;

            case interface_soundcard:

                src_hops_close_interface_soundcard(obj);

            break;

            case interface_pulseaudio:

                src_hops_close_interface_pulseaudio(obj);

            break;

            default:

                printf("Source hops: Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;

        }

    }

    void src_hops_close_interface_file(src_hops_obj * obj) {

        fclose(obj->fp);

    }

    void src_hops_close_interface_soundcard(src_hops_obj * obj) {

        snd_pcm_close(obj->ch);

    }

    void src_hops_close_interface_pulseaudio(src_hops_obj * obj) {

        if (obj->pa != NULL)
            pa_simple_free(obj->pa);

    }

    int src_hops_process(src_hops_obj * obj) {

        int rtnValue;

        switch(obj->format->type) {

            case format_binary_int08:

                src_hops_process_format_binary_int08(obj);

            break;

            case format_binary_int16:

                src_hops_process_format_binary_int16(obj);                

            break;

            case format_binary_int24:

                src_hops_process_format_binary_int24(obj);

            break;

            case format_binary_int32:

                src_hops_process_format_binary_int32(obj);

            break;

            default:

                printf("Source hops: Invalid format type.\n");
                exit(EXIT_FAILURE);

            break;

        }

        switch(obj->interface->type) {

            case interface_file:

                rtnValue = src_hops_process_interface_file(obj);

            break;

            case interface_soundcard:

                rtnValue = src_hops_process_interface_soundcard(obj);

            break;

            case interface_pulseaudio:

                rtnValue = src_hops_process_interface_pulseaudio(obj);

            break;

            default:

                printf("Source hops: Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;           

        }

        obj->timeStamp++;
        obj->out->timeStamp = obj->timeStamp;

        return rtnValue;

    }

    int src_hops_process_interface_file(src_hops_obj * obj) {

        unsigned int nBytesTotal;
        int rtnValue;

        nBytesTotal = fread(obj->buffer, sizeof(char), obj->bufferSize, obj->fp);

        if (nBytesTotal == obj->bufferSize) {
            
            rtnValue = 0;    

        }
        else {

            rtnValue = -1;

        }

        return rtnValue;

    }

    int src_hops_process_interface_soundcard(src_hops_obj * obj) {

        unsigned int nSamples;
        int rtnValue;
        int err;

        if (err = snd_pcm_readi(obj->ch, obj->buffer, obj->hopSize) > 0) {
            
            rtnValue = 0;    

        }
        else {

            rtnValue = -1;

        }

        return rtnValue;

    }

    int src_hops_process_interface_pulseaudio(src_hops_obj * obj) {

        int rtnValue;
        int err;

        if (pa_simple_read(obj->pa, obj->buffer, obj->bufferSize, &err) < 0) {
            
            rtnValue = -1;

        }
        else {

            rtnValue = 0;

        }

        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        unsigned long long microseconds = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;

        // remove pulseaudio latency
        pa_usec_t latency;
        latency = pa_simple_get_latency(obj->pa, &err);
        if (latency == (pa_usec_t) -1) {
            printf("Source hops: Could not get pulseaudio latency: %s\n", pa_strerror(err));
            rtnValue = -1;
        }

        unsigned long long timestamp = obj->timeStamp + 1;
        unsigned long long readTimeStamp = microseconds - latency;

        const int n = snprintf(NULL, 0, "%llu", timestamp);
        char key[n+1];
        int c = snprintf(key, n+1, "%llu", timestamp);
        if (key[n] != '\0' || c != n) {
            printf("Source hops: Invalid timestamp \n");
            rtnValue = -1;
        }

        if (obj->timeStampMap != NULL) {
            map_set(obj->timeStampMap, key, readTimeStamp);
        }

        return rtnValue;

    }

    void src_hops_process_format_binary_int08(src_hops_obj * obj) {

        unsigned int iSample;
        unsigned int iChannel;
        unsigned int nBytes;
        float sample;

        nBytes = 1;

        for (iSample = 0; iSample < obj->hopSize; iSample++) {

            for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                memcpy(&(obj->bytes[4-nBytes]),
                       &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
                       sizeof(char) * nBytes);

                sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

                obj->out->hops->array[iChannel][iSample] = sample;

            }

        }

    }

    void src_hops_process_format_binary_int16(src_hops_obj * obj) {

        unsigned int iSample;
        unsigned int iChannel;
        unsigned int nBytes;
        float sample;

        nBytes = 2;

        for (iSample = 0; iSample < obj->hopSize; iSample++) {

            for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                memcpy(&(obj->bytes[4-nBytes]),
                       &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
                       sizeof(char) * nBytes);

                sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

                obj->out->hops->array[iChannel][iSample] = sample;

            }

        }

    }

    void src_hops_process_format_binary_int24(src_hops_obj * obj) {

        unsigned int iSample;
        unsigned int iChannel;
        unsigned int nBytes;
        float sample;

        nBytes = 3;

        for (iSample = 0; iSample < obj->hopSize; iSample++) {

            for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                memcpy(&(obj->bytes[4-nBytes]),
                       &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
                       sizeof(char) * nBytes);

                sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

                obj->out->hops->array[iChannel][iSample] = sample;

            }

        }

    }

    void src_hops_process_format_binary_int32(src_hops_obj * obj) {

        unsigned int iSample;
        unsigned int iChannel;
        unsigned int nBytes;
        float sample;

        nBytes = 4;

        for (iSample = 0; iSample < obj->hopSize; iSample++) {

            for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                memcpy(&(obj->bytes[4-nBytes]),
                       &(obj->buffer[(iSample * obj->nChannels + iChannel) * nBytes]),
                       sizeof(char) * nBytes);

                sample = pcm_signedXXbits2normalized(obj->bytes, nBytes);

                obj->out->hops->array[iChannel][iSample] = sample;

            }

        }

    }

    void src_hops_set_timestamp_map(src_hops_obj * obj, map_ull_t * timestamp_map)
    {
        obj->timeStampMap = timestamp_map;
    }

    src_hops_cfg * src_hops_cfg_construct(void) {

        src_hops_cfg * cfg;

        cfg = (src_hops_cfg *) malloc(sizeof(src_hops_cfg));

        cfg->format = (format_obj *) NULL;
        cfg->interface = (interface_obj *) NULL;

        return cfg;

    }

    void src_hops_cfg_destroy(src_hops_cfg * src_hops_config) {

        if (src_hops_config->format != NULL) {
            format_destroy(src_hops_config->format);
        }
        if (src_hops_config->interface != NULL) {
            interface_destroy(src_hops_config->interface);
        }

        free((void *) src_hops_config);

    }
