#include "io.h"
#include "containers.h"
#include "file.h"
#include <ctype.h>
#include <stdio.h>

namespace {
class video_shader_file : public video::shader {
public:
struct pass_t * passes;
struct pass_t pass_clone;

const struct pass_t * pass(unsigned int n, lang_t language)
{
	if (language==this->passes[n].lang) return &this->passes[n];
	else
	{
		this->pass_clone=this->passes[n];
		this->pass_clone.lang=language;
		this->pass_clone.source=translate(this->passes[n].lang, language, this->passes[n].source);
		return &this->pass_clone;
	}
}

void pass_free(const struct pass_t * pass)
{
	if (pass==&this->pass_clone) free((char*)this->pass_clone.source);
}



struct tex_t * textures;
char* * texpaths;

const struct tex_t * texture(unsigned int n)
{
	//TODO: load data
	return &textures[n];
}

void texture_free(const struct tex_t * texture)
{
	
}

/*private*/ wrap_t parse_wrap(const char * name, bool* error=NULL)
{
	if (!strcmp(name, "clamp_to_border")) return wr_border;
	if (!strcmp(name, "clamp_to_edge")) return wr_edge;
	if (!strcmp(name, "repeat")) return wr_repeat;
	if (!strcmp(name, "mirrored_repeat")) return wr_mir_repeat;
	
	if (error) *error=true;
	return wr_border;
}

/*private*/ scale_t parse_scale(const char * name, bool* error=NULL)
{
	if (!strcmp(name, "source")) return sc_source;
	if (!strcmp(name, "viewport")) return sc_viewport;
	if (!strcmp(name, "absolute")) return sc_absolute;
	
	if (error) *error=true;
	return sc_source;
}

/*private*/ bool construct(const char * filename)
{
	const char * ext=strrchr(filename, '.');
	if (!ext || strchr(ext, '/')) return NULL;
	ext++;
	
	this->n_pass=0;
	this->passes=NULL;
	
	this->n_tex=0;
	this->textures=NULL;
	this->texpaths=NULL;
	
	unsigned int extlen=strlen(ext);
	bool has_preset=(tolower(ext[extlen-1])=='p');
	if (has_preset) extlen--;
	
	lang_t lang;
	if(0);
	else if (extlen==4 && !strncasecmp(ext, "glsl", extlen)) lang=la_glsl;
	else if (extlen==2 && !strncasecmp(ext, "cg", extlen))   lang=la_cg;
	//else if (extlen==4 && !strncasecmp(ext, "hlsl", extlen)) lang=la_hlsl;
	else return false;
	
	char * passdata;
	if (has_preset)
	{
		if (!file_read(filename, (void**)&passdata, NULL)) return false;
	}
	else
	{
		//this could be handled without going through the config parser at all, but synthesizing one
		// gives less duplicate code and therefore less potential errors
		asprintf(&passdata, "shaders=1\nshader0=%s", filename);
		filename=NULL;
	}
	
	config cfg(passdata);
	free(passdata);
	
	bool error=false;
	
cfg.g();
	if (!cfg.read("shaders", &this->n_pass) || !this->n_pass) return false;
	this->passes=malloc(sizeof(pass_t)*this->n_pass);
	memset(this->passes, 0, sizeof(pass_t)*this->n_pass);
	
	for (unsigned int i=0;i<this->n_pass;i++)
	{
		char tmp[32];
#define read_pass(prefix, loc) (sprintf(tmp, prefix"%i", i) && cfg.read(tmp, loc, &error))
		const char * subname;
		if (!read_pass("shader", &subname)) return false;
		if (!file_read_rel(filename, !has_preset, subname, (void**)&this->passes[i].source, NULL)) return false;
		this->passes[i].lang=lang;
		
		bool linear=false;
		read_pass("filter_linear", &linear);
		this->passes[i].interpolate=(linear ? in_linear : in_nearest);
		
		const char * wrap="clamp_to_border";
		read_pass("wrap_mode", &wrap);
		this->passes[i].wrap=parse_wrap(wrap, &error);
		
		this->passes[i].frame_max=0;
		read_pass("frame_count_mod", &this->passes[i].frame_max);
		this->passes[i].mipmap_input=false;
		read_pass("mipmap_input", &this->passes[i].mipmap_input);
		
		bool fbo_srgb=false;
		read_pass("srgb_framebuffer", &fbo_srgb);
		bool fbo_float=false;
		read_pass("float_framebuffer", &fbo_float);
		if (fbo_srgb && fbo_float) return false;
		this->passes[i].fboformat=(fbo_srgb ? fb_srgb : fbo_float ? fb_float : fb_int);
		
		//there's a mysterious entry known as 'alias', of which type and use is unknown (likely string and affecting shader parsing somehow)
		
		this->passes[i].scale_x=1.0;
		bool has_scale_both=read_pass("scale", &this->passes[i].scale_x);
		bool has_scale_x=read_pass("scale_x", &this->passes[i].scale_x);
		bool has_scale_y=read_pass("scale_y", &this->passes[i].scale_y);
		if ((has_scale_both && has_scale_x) || has_scale_x != has_scale_y) return false;
		if (!has_scale_x) this->passes[i].scale_y=this->passes[i].scale_x;
		
		const char * scale_type_x="source";
		const char * scale_type_y;
		has_scale_both=read_pass("scale_type", &scale_type_x);
		has_scale_x=read_pass("scale_type_x", &scale_type_x);
		has_scale_y=read_pass("scale_type_y", &scale_type_y);
		if ((has_scale_both && has_scale_x) || has_scale_x != has_scale_y) return false;
		if (!has_scale_x) scale_type_y=scale_type_x;
		this->passes[i].scale_type_x=parse_scale(scale_type_x, &error);
		this->passes[i].scale_type_y=parse_scale(scale_type_y, &error);
#undef read_pass
		
		if (error) return false;
	}
	
	//TODO: textures
	//TODO: imports
	//TODO: parameters (scan the shader sources)
	
	if (!cfg.all_used()) return false;
	
	return true;
}

~video_shader_file()
{
	free(this->passes);
	for (unsigned int i=0;i<this->n_tex;i++)
	{
		free((char*)this->textures[i].name);
		free(this->texpaths[i]);
	}
	free(this->textures);
	free(this->texpaths);
}

};

}

video::shader* video::shader::create_from_file(const char * filename)
{
	video_shader_file* ret=new video_shader_file();
	if (!ret->construct(filename))
	{
		delete ret;
		return NULL;
	}
	return ret;
}

#if 0
video::shader* video::shader::create_from_file(const char * filename)
{
	const char * ext=strrchr(filename, '.');
	if (!ext || strchr(ext, '/')) return NULL;
	ext++;
	shader* ret=malloc(sizeof(struct shader));
	ret->pass=NULL; ret->n_pass=0;
	ret->param=NULL; ret->n_param=0;
	
	unsigned int extlen=strlen(ext);
	bool multipass=(tolower(ext[extlen-1])=='p');
	if (multipass) extlen--;
	char * passdata=NULL;
	
	if(0);
	else if (extlen==4 && !strncasecmp(ext, "glsl", extlen)) ret->type=shader::ty_glsl;
	else if (extlen==2 && !strncasecmp(ext, "cg", extlen)) ret->type=shader::ty_cg;
	else if (extlen==4 && !strncasecmp(ext, "hlsl", extlen)) ret->type=shader::ty_hlsl;
	else goto error;
	
	if (multipass)
	{
		if (!file_read(filename, (void**)&passdata, NULL)) goto error;
	}
	else
	{
		asprintf(&passdata,
			"shaders=1\n"
			"shader0=%s\n"
			//"scale_type0=source\n"
			//"scale0=1.0\n"
			//"filter_linear0=false"
			, filename);
	}
	
	char* name;
	name=passdata;
	
	unsigned int n_pass; n_pass=0;
	shader::pass_t* pass; pass=NULL;
	
	while (true)
	{
		char* lineend=strchr(name, '\n');
	{
		if (lineend) *lineend='\0';
		while (isspace(*name)) name++;
		if (*name=='\0') goto nextline;
		if (*name=='#') goto nextline;
		if (!isalpha(*name)) goto error;
		char* val=strchr(name, '=');
		if (!val) goto error;
		char* nameend=val;
		while (isspace(nameend[-1])) nameend--;
		*nameend='\0';
		val++;
		while (isspace(*val)) val++;
		char* trimwhite=lineend;
		while (isspace(trimwhite[-1])) trimwhite--;
		*trimwhite='\0';
		
		if(0);
		
		else if (!strcmp(name, "shaders"))
		{
			if (ret->pass) goto error;//this can only exist once
			char* end;
			n_pass=strtoul(val, &end, 10);
			if (n_pass<=0 || end==val || *end) goto error;
			pass=malloc(sizeof(shader::pass_t)*ret->n_pass);
			ret->n_pass=n_pass;
			ret->pass=pass;
			for (unsigned int i=0;i<n_pass;i++)
			{
				static const shader::pass_t def_pass={
					/*source*/      NULL,
					/*interpolate*/ shader::in_nearest,
					/*wrap*/        shader::wr_border,
				};
				pass[i]=def_pass;
			}
			goto nextline;
		}
		
		else if (isdigit(nameend[-1]))
		{
			if (!pass) goto error;//shaders= must come before these
			char* pass_id_str=nameend;
			while (isdigit(pass_id_str[-1])) pass_id_str--; // this is known to terminate - there's an isalpha() check earlier, and nothing is both letter and number
			char* end;
			unsigned int pass_id=strtoul(pass_id_str, &end, 10);
			if (end==pass_id_str || *end) goto error;
			*pass_id_str='\0';
			
			//bool - {0, 1, true, false}
			if(0);
			else if (!strcmp(name, "shader"))
			{
				if (pass[pass_id].source) goto error;
				if (!file_read_rel(filename, false, val, (void**)&pass[pass_id].source, NULL)) goto error;
			}
			else if (!strcmp(name, "filter_linear"))
			{
				//bool
			}
			else if (!strcmp(name, "wrap_mode"))
			{
				//{clamp_to_border, clamp_to_edge, repeat, mirrored_repeat}
			}
			else if (!strcmp(name, "frame_count_mod"))
			{
				//int
			}
			if (!strcmp(name, "srgb_framebuffer"))
			{
				//bool
			}
			if (!strcmp(name, "float_framebuffer"))
			{
				//bool
			}
			if (!strcmp(name, "mipmap_input"))
			{
				//bool
			}
			if (!strcmp(name, "alias"))
			{
				//string?
			}
			if (!strncmp(name, "scale_type", strlen("scale_type")))
			{
				//scale_type, scale_type_x, scale_type_y
				//{source, viewport, absolute}
			}
			if (!strncmp(name, "scale", strlen("scale")))
			{
				//scale, scale_x, scale_y
				//float, except if scalemode==absolute, in which case they're int
			}
			else goto error;
		}
		
		else if (!strcmp(name, "textures"))
		{
			//semicolon-separated list of strings
			
		}
		//for each texture:
		//%_linear - bool
		//%_wrap_mode - 
		//%_mipmap - 
		
		else if (!strcmp(name, "imports"))
		{
			//semicolon-separated list of strings
			//for game-aware shaders?
			//Python stuff seems painful.
		}
		//for each import:
		//%_semantic - {capture, transition, transition_count, capture_previous, transition_previous, python}
		//%_input_slot - {1, 2}
		//%_wram - hex
		//%_mask - hex
		//%_equal - hex
		//additionally:
		//import_script - string
		//import_script_class - string
		//likely used only for semantic=python
		//
		// def fetch:
		//  u16 ret;
		//  if exist %_input_slot: ret=game::input[slot] as uint16;
		//  else: ret=game::wram[%_wram] as uint8;
		//  ret &= %_mask
		//  if exist %_equal: ret=(ret==equal);
		//  return ret;
		// 
		// def delay(new):
		//  static u32 prev;
		//  static u32 val;
		//  if (val!=new):
		//   prev=val;
		//   val=new;
		//  return prev;
		//
		//if semantic is:
		// capture:
		//  return fetch()
		// transition:
		//  static u16 prev;
		//  static u32 prev_framecount;
		//  if (fetch() != prev):
		//   prev=fetch();
		//   prev_framecount=game::framecount;
		//  return prev_framecount;
		// transition_count:
		//  static u16 prev;
		//  static u32 prev_count;
		//  if (fetch() != prev):
		//   prev=fetch();
		//   prev_count++;
		//  return prev_count;
		// capture_previous:
		//  return delay(capture());
		// transition_previous:
		//  return delay(transition());
		// python: ???
		
		else goto error;
	}
	nextline:
		if (!lineend) break;
		name=lineend+1;
	}
	
	return ret;
	
error:
	shader_delete(ret);
	free(passdata);
	return NULL;
}

video::shader::~shader()
{
	
}
#endif