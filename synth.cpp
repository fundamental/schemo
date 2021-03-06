#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <complex>
#include <stdint.h>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <rtosc/thread-link.h>
#include <rtosc/undo-history.h>
#include <rtosc/miditable.h>


using rtosc::Ports;

/*****************
 * RTOSC Globals *
 *****************/
rtosc::ThreadLink bToU(1024,20);
rtosc::ThreadLink uToB(1024,20);
rtosc::UndoHistory undo;
rtosc::MidiMapperRT midi_rt;
rtosc::MidiMappernRT midi_nrt;


/**************
 * Parameters *
 **************/
#define rUnit(x) rMap(unit, x)


struct EnvPars {
    float volume = 0.0;   //dB
    float atime  = 100.0; //ms
    float dtime  = 10.0;  //ms
    float svalue = -2.0;  //dB
    float rtime  = 200.0; //ms
    static Ports ports;
};

struct MixPars {
    float pan1 = 0.5;
    float pan2 = 0.5;
    float level1 = 1.0;
    float level2 = 1.0;
    static Ports ports;
};

struct OscilPars {
    float mix_sin = 0.0;
    float mix_sqr = 0.0;
    float mix_saw = 1.0;
    float detune  = 0.0;
    bool sync     = true;
    static Ports ports;
};

#define rObject EnvPars
Ports EnvPars::ports = {
    rParamF(volume, rLinear(-4,0),    rUnit(dB), "Max Volume"),
    rParamF(atime, rLinear(0,1000.0), rUnit(ms), "Attack Time"),
    rParamF(dtime, rLinear(0,1000.0), rUnit(ms), "Decay Time"),
    rParamF(rtime, rLinear(0,1000.0), rUnit(ms), "Release Time"),
    rParamF(svalue, rLinear(-10,0),   rUnit(dB), "Sustain Value"),
};
#undef rObject
#define rObject MixPars
Ports MixPars::ports {
    rParamF(pan1, rLinear(0,1),   rUnit(panning), "Oscillator 1 Panning"),
    rParamF(pan2, rLinear(0,1),   rUnit(panning), "Oscillator 2 Panning"),
    rParamF(level1, rLinear(0,1), rUnit(percent), "Oscillator 1 Relative Level"),
    rParamF(level2, rLinear(0,1), rUnit(percent), "Oscillator 2 Relative Level"),
};
#undef rObject
#define rObject OscilPars
Ports OscilPars::ports {
    rParamF(mix_sin, rLinear(0,1), rUnit(percent), "Sin Mixture"),
    rParamF(mix_sqr, rLinear(0,1), rUnit(percent), "Square Mixture"),
    rParamF(mix_saw, rLinear(0,1), rUnit(percent), "Sawtooth Mixture"),
    rParamF(detune,  rLinear(-2400,2400), rUnit(cents), "Detune Amount"),
    rToggle(sync, "Sync to another Oscillator"),
};
#undef rObject


/****************
 * Note storage *
 ****************/
struct NoteActivity
{
    short notes[128] = {0};//store the sort perm
    char playing = 0;
    void activate(short n)
    {
        short old_note = notes[n];
        if(old_note) {
            for(int i=0; i<128; ++i)
                if(notes[i] > old_note)
                    notes[i]--;
            notes[n] = playing;
        } else
            notes[n] = ++playing;
    }
    char deactivate(char n)
    {
        char old_note = notes[(int)n];
        if(old_note) {
            for(int i=0; i<128; ++i)
                if(notes[i] > old_note)
                    notes[i]--;
            playing--;
        }
        notes[(int) n] = 0;

        char ret = -1;
        for(int i=0; i<128; ++i)
            if((ret == -1 && notes[(int)i]) ||
                    (ret != -1 && notes[(int)i] > notes[(int)ret]))
                ret = i;
        return ret;
    }
};

/*****************
 * State Objects *
 *****************/


enum env_stage
{
    ENV_ATTACK,
    ENV_SUSTAIN,
    ENV_DECAY,
    ENV_RELEASE,
    ENV_IDLE
};

struct EnvState {
    EnvState(EnvPars &ep):pars(ep){}
    float current   = 0.0;
    float coeff     = 0.0;
    env_stage stage = ENV_IDLE;
    int countdown   = 0;
    EnvPars &pars;
};

struct OscilState {
    OscilState(OscilPars &op):pars(op){}
    float phase     = 0.0;
    bool phase_minus = true;
    OscilPars &pars;
};

struct Synth_
{

    void osc_doc(void) {
        puts("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        puts("<osc_unit format_version=\"1.0\">");
        puts(" <meta>");
        puts("  <name>schemo</name>");
        puts("  <uri>http://fundamental-code.com/wiki/schemo/</uri>");
        puts("  <doc_origin>http://where.to/?find=this&amp;xml_file</doc_origin>");
        puts("  <author><firstname>Mark</firstname><lastname>McCurry</lastname></author>");
        puts(" </meta>");
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        rtosc::walk_ports(&ports,
                buffer, 1024, NULL,
                [](const rtosc::Port*p, const char *name, void *) {
                auto meta = p->meta();
                if(meta.find("parameter") != p->meta().end()) {
                    char type = 0;
                    const char *foo = index(p->name, ':');
                    if(index(foo, 'f'))
                        type = 'f';
                    else if(index(foo, 'i'))
                        type = 'i';
                    if(!type) return;

                    printf(" <message_in pattern=\"%s\" typetag=\"%c\">\n", name, type);
                    printf("  <desc>Set Value of %s</desc>\n", p->meta()["documentation"]);
                    if(meta.find("min") != meta.end() && meta.find("max") != meta.end())
                    {
                        printf("  <param_%c symbol=\"x\" units=\"%s\">", type, meta["unit"]);
                        printf("   <range_min_max min=\"%s\" lmin=\"[\" lmax=\"]\" max=\"%s\"/>", meta["min"], meta["max"]);
                        printf("  </param_%c>", type);
                    } else
                        printf("  <param_%c symbol=\"x\" units=\"%s\"/>\n", type, meta["unit"]);
                    printf(" </message_in>\n");
                    printf(" <message_in pattern=\"%s\" typetag=\"\">\n", name);
                    printf("  <desc>Get Value of %s</desc>\n", p->meta()["documentation"]);
                    printf(" </message_in>\n");
                    printf(" <message_out pattern=\"%s\" typetag=\"%c\">\n", name, type);
                    printf("  <desc>Value of %s</desc>\n", p->meta()["documentation"]);
                    if(meta.find("min") != meta.end() && meta.find("max") != meta.end())
                    {
                        printf("  <param_%c symbol=\"x\" units=\"%s\">", type, meta["unit"]);
                        printf("   <range_min_max min=\"%s\" lmin=\"[\" lmax=\"]\" max=\"%s\"/>", meta["min"], meta["max"]);
                        printf("  </param_%c>", type);
                    } else
                        printf("  <param_%c symbol=\"x\" unit=\"%s\"/>\n", type, meta["unit"]);
                    printf(" </message_out>\n");
                }
                });
        printf("</osc_unit>\n");
    }

    Synth_(void)
        :osc1(osc1p), osc2(osc2p), env(envp)
    {
    }
    NoteActivity notes;
    float baseFreq = 440.0;
    bool  gate     = false;
    OscilPars  osc1p, osc2p;
    OscilState osc1, osc2;
    EnvState   env;
    EnvPars    envp;
    MixPars    mix;
    static Ports ports;
};

void send_cc(int cc, int val);
#define rObject Synth_
Ports Synth_::ports = {
    rRecur(osc1p, "First Oscillator"),
    rRecur(osc2p, "Second Oscillator"),
    rRecur(envp,  "Envelope"),
    rRecur(mix,   "Mixture"),
    {"undo_pause", 0, 0, [](const char *, rtosc::RtData &d)
        {d.reply("/undo_pause","");}},
    {"undo_resume", 0, 0, [](const char *, rtosc::RtData &d)
        {d.reply("/undo_resume","");}},
    midi_rt.addWatchPort(),
    midi_rt.removeWatchPort(),
    midi_rt.bindPort(),
    {"virtual_midi_cc:ii", 0, 0, [](const char *msg, rtosc::RtData &d)
        {
            (void) d;
            int val = rtosc_argument(msg, 0).i;
            int cc = rtosc_argument(msg, 1).i;
            send_cc(cc,val);
            midi_rt.handleCC(cc,val);
        }},

};


/***********
 * Globals *
 ***********/

#define OSCIL_SIZE 512
static float sqr_table[12][OSCIL_SIZE];
static float saw_table[12][OSCIL_SIZE];
static float sin_table[12][OSCIL_SIZE];
static float table_freqs[12];
float sampleRate = 0.0;

/********************
 * Normal Functions *
 ********************/

//0 V1
//T V2
//f(x) = exp(ln(v1)+(ln(v2)-ln(v1))*x/T)
//f(x) = exp((1-x/T)ln(v1)+ln(v2)*x/T)
//f(x) = expf(a+bx)
//f(x) = A e^bx
//df/dx = A e^bx b
//f(x+delta) = delta*b*f(x)+f(x)
//f(x+delta) = (delta*b+1)*f(x)
//f(x+1) = C*f(x)
//C=1+b=1+ln(vs/v1)*Fs/T
float get_exp_coeff(float Fs, float ms, float v1, float v2)
{
    return 1.0+log(v2/v1)/(Fs*ms*1e-3);
}

void create_wavetables(int type, float Fs, float tables[12][OSCIL_SIZE], float freqs[12])
{
    //static_assert((int)(&tables[0][1] - &tables[0][0]) == 4, "table alignment");
    if(type == 0)
        for(int i=0; i<OSCIL_SIZE; ++i)
            tables[0][i] = sin(2*M_PI*i*1.0/OSCIL_SIZE);
    else if(type == 1)
        for(int i=0; i<OSCIL_SIZE; ++i)
            tables[0][i] = 1.0-i*2.0/OSCIL_SIZE;
    else if(type == 2)
        for(int i=0; i<OSCIL_SIZE; ++i)
            tables[0][i] = i < OSCIL_SIZE/2 ? 1.0 : -1.0;

    //TODO improve harmonic content within range by allowing non-audible
    //aliasing in a more exact manner
    freqs[0] = Fs/OSCIL_SIZE;

    //DFT (ignore first bin as this should all be zero mean)
    std::complex<float> Table[OSCIL_SIZE/2] = {0};
    for(int i=0; i<OSCIL_SIZE/2; ++i)
        for(int j=0; j<OSCIL_SIZE; ++j)
            Table[i] += std::polar(tables[0][j], (float)(-j*(1+i)*2*M_PI/OSCIL_SIZE));

    float scale = 1;
    for(int i=1; i<12; ++i) {
        scale *= 2.0;
        memset(tables[i], 0, OSCIL_SIZE*sizeof(float));
        //Restricted IDFT
        for(int j=0; j<OSCIL_SIZE; ++j)              //small % alias at threshold
            for(int k=0; k<OSCIL_SIZE/2 && (1.0+k)/OSCIL_SIZE < 0.75/scale; ++k)
                tables[i][j] += 2.0/OSCIL_SIZE*
                    (Table[k]*
                     std::polar(1.0f,(float)((1+k)*j*2*M_PI/OSCIL_SIZE))).real();
        freqs[i] = scale*freqs[0];
    }
};

float noteToFreq(int x)
{
    return powf(2.0, (x-69)/12.0f)*440.0;
}

float dBtoRaw(float x)
{
    return powf(10.0f, x/20.0);
}

void env_handle_gate(EnvState &env, bool gate)
{
    if(!gate && (env.stage == ENV_ATTACK || env.stage == ENV_SUSTAIN)) {
        env.stage = ENV_RELEASE;
        env.countdown = sampleRate*1e-3*env.pars.rtime;
        env.coeff = get_exp_coeff(sampleRate,
                        env.pars.rtime, env.current,
                        1e-6);
    }

    if(gate && (env.stage == ENV_IDLE || env.stage == ENV_RELEASE)) {
        env.stage = ENV_ATTACK;
        env.current = 1e-6;
        env.countdown = sampleRate*1e-3*env.pars.atime;
        env.coeff = get_exp_coeff(sampleRate,
                        env.pars.atime, env.current,
                        1.0);
    }
}

void env_out(float *cv,  EnvState &env, unsigned smps)
{
    float scale = dBtoRaw(env.pars.volume);
    unsigned i = 0;
    //Finite state automita, aka goto is normal
new_state:
    //TODO tidy up
    switch(env.stage)
    {
        case ENV_ATTACK:
            for(; i<smps && env.countdown; ++i, env.countdown--)
                cv[i] = scale*(env.current *= env.coeff);
            if(!env.countdown) {
                env.stage = ENV_DECAY;
                env.countdown = sampleRate*1e-3*env.pars.dtime;
                env.coeff = get_exp_coeff(sampleRate,
                        env.pars.dtime, env.current,
                        dBtoRaw(env.pars.svalue));
                if(i) goto new_state;
            }
            break;
        case ENV_DECAY:
            for(; i<smps && env.countdown; ++i, env.countdown--)
                cv[i] = scale*(env.current *= env.coeff);
            if(!env.countdown) {
                env.stage = ENV_SUSTAIN;
                env.coeff = 1.0;
                if(i) goto new_state;
            }
            break;
        case ENV_SUSTAIN:
            for(; i<smps; ++i)
                cv[i] = scale*env.current;
            break;
        case ENV_RELEASE:
            for(; i<smps && env.countdown; ++i, env.countdown--)
                cv[i] = scale*(env.current *= env.coeff);
            if(!env.countdown) {
                env.stage = ENV_IDLE;
                env.coeff = 1.0;
                if(i) goto new_state;
            }
            break;
        case ENV_IDLE:
            for(; i<smps; ++i)
                cv[i] = 0.0;
            env.coeff = 0.0;
            env.current = 1e-6;
            break;
    }
}

//linear interpolation
float interpolate(float *smps, unsigned nsmps, float pos)
{
    float val  = nsmps*pos;
    int   posa = (int)val;
    int   posb = (posa + 1)%nsmps;
    float lerp = fmodf(val, 1.0);
    return smps[posa]*(1.0-lerp)+smps[posb]*lerp;
}

void run_osc(float *out, const float *sync, OscilState &osc,
             float freq, unsigned smps)
{
    float fq    = freq*powf(2.0, osc.pars.detune/1200.0);
    int table = 11;
    for(int i=0; i<12 && table == 11; ++i)
        if(fq < table_freqs[i])
            table = i;

    float delta = fq/sampleRate;
    float mix_sin = osc.pars.mix_sin;
    float mix_saw = osc.pars.mix_saw;
    float mix_sqr = osc.pars.mix_sqr;
    float scale   = mix_sin+mix_saw+mix_sqr;
    if(scale > 1e-4) {
        mix_sin /= scale;
        mix_saw /= scale;
        mix_sqr /= scale;
    }

    if(sync && osc.pars.sync) {
        for(unsigned i=0; i<smps; ++i)
        {
            osc.phase += delta;
            if(osc.phase > 1.0)
                osc.phase -= 1;

            if(osc.phase_minus && sync[i] > 0)
                osc.phase = 0;
            osc.phase_minus = sync[i] < 0;


            out[i] = 0;
            out[i] += mix_sin*interpolate(sin_table[table], OSCIL_SIZE, osc.phase);
            out[i] += mix_saw*interpolate(saw_table[table], OSCIL_SIZE, osc.phase);
            out[i] += mix_sqr*interpolate(sqr_table[table], OSCIL_SIZE, osc.phase);
        }
    } else {
        for(unsigned i=0; i<smps; ++i) {
            osc.phase += delta;
            if(osc.phase > 1.0)
                osc.phase -= 1;
            out[i] = 0;
            out[i] += mix_sin*interpolate(sin_table[table], OSCIL_SIZE, osc.phase);
            out[i] += mix_saw*interpolate(saw_table[table], OSCIL_SIZE, osc.phase);
            out[i] += mix_sqr*interpolate(sqr_table[table], OSCIL_SIZE, osc.phase);
        }
    }
}


Synth_ synth;
void process(float *outl, float *outr, unsigned smps)
{
    float p1 = synth.mix.pan1;
    float p2 = synth.mix.pan2;
    float l1 = synth.mix.level1;
    float l2 = synth.mix.level2;

    float gain[smps];
    float osc1[smps];
    float osc2[smps];
    run_osc(osc1, NULL, synth.osc1, synth.baseFreq, smps);
    run_osc(osc2, osc1, synth.osc2, synth.baseFreq, smps);
    env_handle_gate(synth.env, synth.gate);
    env_out(gain, synth.env, smps);

    for(unsigned i=0; i<smps; ++i) {
        outl[i] = gain[i]*(    p1*l1*osc1[i]+    p2*l2*osc2[i])/2;
        outr[i] = gain[i]*((1-p1)*l1*osc1[i]+(1-p2)*l2*osc2[i])/2;
    }
}

void noteOn(Synth_ &synth, char note)
{
    synth.notes.activate(note);
    synth.baseFreq = noteToFreq(note);
    synth.gate = true;
}

void noteOff(Synth_ &synth, char note)
{
    int new_note = synth.notes.deactivate(note);
    if(new_note != -1) {
        synth.baseFreq = noteToFreq(new_note);
    } else {
        synth.gate = false;
    }

}
/***************************
 * Communication And RTOSC *
 ***************************/


class DispatchData:public rtosc::RtData
{
    public:
        DispatchData(void)
        {
            memset(buffer, 0, 1024);
            loc = buffer;
            loc_size = 1024;
            obj = &synth;
        }
        char buffer[1024];

        void reply(const char *path, const char *args, ...)
        {
            va_list va;
            va_start(va,args);
            const size_t len =
                rtosc_vmessage(bToU.buffer(),bToU.buffer_size(),path,args,va);
            if(len)
                bToU.raw_write(bToU.buffer());
        }

        void reply(const char *msg)
        {
            bToU.raw_write(msg);
        }

        void broadcast(const char *path, const char *args, ...)
        {
            bToU.write("/broadcast","");
            va_list va;
            va_start(va,args);
            const size_t len =
                rtosc_vmessage(bToU.buffer(),bToU.buffer_size(),path,args,va);
            if(len)
                bToU.raw_write(bToU.buffer());
        }


        void broadcast(const char *msg)
        {
            bToU.write("/broadcast","");
            bToU.raw_write(msg);
        }

};

void handleEvents(void)
{
    DispatchData d;
    d.matches = 0;
    while(uToB.hasNext()) {
        Synth_::ports.dispatch(uToB.read()+1, d);
        //fprintf(stderr, "backend '%s'\n", uToB.peak());
        if(d.matches == 0)
            fprintf(stderr, "MISSING ADDRESS '%s'\n", uToB.peak());
        d.matches = 0;
    }
}

/*************
 * UI Magics *
 *************/
const rtosc::Port *getPort(rtosc::Ports &p, const char *path)
{
    return p.apropos(path);
}
char portType(const char *pattern)
{
    const char *nstr = index(pattern, ':');
    assert(nstr);
    for(int i=0; i<(int)strlen(nstr); ++i)
    {
        switch(nstr[i])
        {
            case 'f':
            case 'T':
                return nstr[i];
        }
    }
    return 0;
}

void writeNormFloat(const char *addr, float f)
{
    assert(addr && addr[0]=='/');
    const rtosc::Port *p = getPort(Synth_::ports, addr);
    if(!p) {
        fprintf(stderr, "Invalid url '%s'\n", addr);
        return;
    }
    char type = portType(p->name);
    if(type == 'T') {
        uToB.write(addr, f>0.5?"T":"F");
        midi_nrt.snoop(uToB.buffer());
    } else if(type == 'f') {
        //always assume linear map [FIXME]
        float min_ = atof(p->meta()["min"]);
        float max_ = atof(p->meta()["max"]);
        uToB.write(addr,"f", min_+(max_-min_)*f);
        midi_nrt.snoop(uToB.buffer());
    }
}

std::string formatter(std::string unit, float val)
{
    char buffer[1024] ={0};
    if(unit == "percent")
        snprintf(buffer, 1023, "%.1f%%",100*val);
    else if(unit == "dB")
        snprintf(buffer, 1023, "%.2f dB", val);
    else if(unit == "cents")
        snprintf(buffer, 1023, "%d cents", (int)val);
    else if(unit == "ms")
        snprintf(buffer, 1023, "%d ms", (int)val);
    else
        snprintf(buffer, 1023, "(NULL)");

    return buffer;

}

bool recording_undo = true;
void handleUpdates(std::function<void(const char *addr, std::string, float)> cb,
                   std::function<void()> damage_undo,
                   std::function<void()> damage_midi)
{
    using rtosc::msg_t;
    using rtosc::Port;
    while(bToU.hasNext()) {
        //assume all messages are UI only FIXME
        msg_t msg = bToU.read();
        if(!strcmp("/undo_pause", msg)) {
            recording_undo = false;
            continue;
        } else if(!strcmp("/undo_resume", msg)) {
            recording_undo = true;
            continue;
        } else if(!strcmp("undo_change", msg)) {
            if(recording_undo) {
                undo.recordEvent(msg);
                damage_undo();
            }
            continue;
        } else if(!strcmp("/midi-use-CC", msg)) {
            printf("Using an id and forming a connection\n");
            midi_nrt.useFreeID(rtosc_argument(msg, 0).i);
            damage_midi();
            continue;
        } else if(!strcmp("/broadcast", msg))
            continue;
        const Port *p = getPort(Synth_::ports, msg);
        assert(p);
        char type = portType(p->name);
        if(type == 'T') {
            if(rtosc_argument(msg,0).T)
                cb(msg, "On", 1.0);
            else
                cb(msg, "Off", 0.0);
        } else if(type == 'f') {
            //always assume linear map [FIXME]
            float v    = rtosc_argument(msg,0).f;
            float min_ = atof(p->meta()["min"]);
            float max_ = atof(p->meta()["max"]);
            cb(msg, formatter(p->meta()["unit"],v), (v-min_)/(max_-min_));
        }
    }
}

rtosc::UndoHistory *getHistory(void)
{
    return &undo;
}

std::map<std::string,std::string> getMidiMap(void)
{
    //printf("Midi map size = '%d'\n", midi_nrt.getMidiMappingStrings().size());
    return midi_nrt.getMidiMappingStrings();
}

void tryMap(std::string str)
{
    auto map = midi_nrt.getMidiMappingStrings();
    if(map.find(str) != map.end())
        midi_nrt.map(str.c_str(), false);
    else
        midi_nrt.map(str.c_str(), true);
}

rtosc::MidiMappernRT *getMidiMapper(void)
{
    return &midi_nrt;
}

void renderWave(float *out, unsigned nsmps, float a, float b, float c)
{
    float delta = 1.0/nsmps;
    float mix_sin = a;
    float mix_saw = b;
    float mix_sqr = c;
    float scale   = mix_sin+mix_saw+mix_sqr;
    if(scale > 1e-4) {
        mix_sin /= scale;
        mix_saw /= scale;
        mix_sqr /= scale;
    }

    for(unsigned i=0; i<nsmps; ++i) {
        out[i] =  mix_sin*interpolate(sin_table[0], OSCIL_SIZE, i*delta);
        out[i] += mix_saw*interpolate(saw_table[0], OSCIL_SIZE, i*delta);
        out[i] += mix_sqr*interpolate(sqr_table[0], OSCIL_SIZE, i*delta);
    }
}

/**********************
 * Jack Specific Code *
 **********************/
#include <jack/jack.h>
#include <jack/midiport.h>
#include <cstdio>

jack_client_t *client;
jack_port_t *input_port;
jack_port_t *midi_out;
void *midi_port_buf;
jack_port_t *output_port_l;
jack_port_t *output_port_r;
void send_cc(int cc, int val)
{
    //printf("cc sending (%d,%d)...\n\n\n", cc, val);
    unsigned char* buffer = jack_midi_event_reserve(midi_port_buf, 0, 3);
    if(buffer) {
        buffer[0] = 0xb0;
        buffer[1] = cc;
        buffer[2] = val;
    }
}

int process_cb(unsigned nframes, void *)
{
	void* port_buf = jack_port_get_buffer(input_port, nframes);
	jack_midi_event_t in_event;
	unsigned event_count = jack_midi_get_event_count(port_buf);
	midi_port_buf = jack_port_get_buffer(midi_out, nframes);
    jack_midi_clear_buffer(midi_port_buf);
    //Just deal with the jitter, this is a demo app...
	jack_midi_event_get(&in_event, port_buf, 0);
	for(unsigned i=0; i<event_count; i++) {
        jack_midi_event_get(&in_event, port_buf, i);
        if((in_event.buffer[0]&0xf0) == 0x90)
            noteOn(synth, in_event.buffer[1]);
        else if((in_event.buffer[0] & 0xf0) == 0x80)
            noteOff(synth, in_event.buffer[1]);
        else if((in_event.buffer[0] & 0xf0) == 0xb0)
            midi_rt.handleCC(in_event.buffer[1], in_event.buffer[2]);
	}

    handleEvents();

	float *outl = (float *) jack_port_get_buffer(output_port_l, nframes);
	float *outr = (float *) jack_port_get_buffer(output_port_r, nframes);
    process(outl,outr,nframes);
    return 0;
}

void die(void *) {exit(1);}
void setup_jack(void)
{
	client = jack_client_open("schemo", JackNullOption, NULL);

	if(!client)
	{
		fprintf(stderr, "Could not connect to jack...\n");
        exit(1);
	}
	
	jack_set_process_callback(client, process_cb, 0);
    sampleRate = jack_get_sample_rate(client);
	jack_on_shutdown(client, die, 0);
	input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE,
            JackPortIsInput, 0);
	midi_out  = jack_port_register(client, "midi_out", JACK_DEFAULT_MIDI_TYPE,
            JackPortIsOutput, 0);
	output_port_l
        = jack_port_register(client, "outl", JACK_DEFAULT_AUDIO_TYPE,
                JackPortIsOutput, 0);
	output_port_r
        = jack_port_register(client, "outr", JACK_DEFAULT_AUDIO_TYPE,
                JackPortIsOutput, 0);
	
    if(jack_activate(client)) {
		fprintf(stderr, "Could not activate jack connection...\n");
        exit(1);
    }

    //Rtosc setup
    undo.setCallback([](const char *msg){
            uToB.write("/undo_pause","");
            midi_nrt.snoop(msg);
            uToB.raw_write(msg);
            uToB.write("/undo_resume","");});

    midi_rt.frontend = [](const char *msg){bToU.raw_write(msg);};
    midi_rt.backend  = [](const char *msg){
        DispatchData d;
        Synth_::ports.dispatch(msg+1, d);};
    midi_nrt.base_ports = &Synth_::ports;
    midi_nrt.rt_cb      = [](const char *msg){uToB.raw_write(msg);};


    //Setup parameters
    synth.osc1.pars.detune = -12;
    synth.osc2.pars.sync   = false;
    create_wavetables(0, sampleRate, sin_table, table_freqs);
    create_wavetables(1, sampleRate, saw_table, table_freqs);
    create_wavetables(2, sampleRate, sqr_table, table_freqs);
}

void close_jack(void)
{
	jack_client_close(client);
}
