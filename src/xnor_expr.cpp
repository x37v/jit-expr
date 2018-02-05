#include <m_pd.h>
#include <string>
#include "llvmcodegen/codegen.h"
#include "parser.hh"
#include <stdexcept>
#include <vector>
#include <memory>
#include <algorithm>

struct _xnor_expr_proxy;
struct _xnor_expr;

namespace {
  const bool print_code = true; //XXX for debugging
  const int buffer_size = 64;

  enum class XnorExpr {
    CONTROL,
    VECTOR,
    SAMPLE
  };

  struct cpp_expr {
    std::vector<t_inlet *> ins;
    std::vector<t_outlet *> outs;
    std::vector<struct _xnor_expr_proxy *> proxies;

    parse::Driver driver;
    xnor::LLVMCodeGenVisitor cv;

    xnor::LLVMCodeGenVisitor::function_t func;
    XnorExpr expr_type = XnorExpr::CONTROL;

    std::vector<float> outfloat;
    std::vector<float *> outarg;
    std::vector<float> infloats;
    std::map<unsigned int, std::vector<float>> saved_inputs;
    std::map<unsigned int, std::vector<float>> saved_outputs;

    std::vector<xnor::LLVMCodeGenVisitor::input_arg_t> inarg;
    std::vector<xnor::ast::Variable::VarType> input_types;
    int signal_inputs = 0; //could just calc from input_types

    //constructor
    cpp_expr(XnorExpr t) : expr_type(t) { };
    ~cpp_expr() {
      for (auto i: ins)
        inlet_free(i);
      ins.clear();

      for (auto i: outs)
        outlet_free(i);
      outs.clear();
    }
  };
}

extern "C" void *xnor_expr_new(t_symbol *s, int argc, t_atom *argv);
extern "C" void xnor_expr_free(struct _xnor_expr * x);
extern "C" void xnor_expr_setup(void);

//functions called from generated code
extern "C" float xnor_expr_factf(float v);
extern "C" float * xnor_expr_table_value_ptr(t_symbol * name, float findex);
extern "C" float xnor_expr_maxf(float a, float b);
extern "C" float xnor_expr_value_assign(t_symbol * name, float v);
extern "C" float xnor_expr_value_get(t_symbol * name);

static t_class *xnor_expr_class;
static t_class *xnor_expr_proxy_class;
static t_class *xnor_expr_tilde_class;
static t_class *xnor_fexpr_tilde_class;

typedef struct _xnor_expr {
  t_object x_obj;
  std::shared_ptr<cpp_expr> cpp;
  float exp_f;   		/* control value to be transformed to signal */
} t_xnor_expr;

typedef struct _xnor_expr_proxy {
	t_pd p_pd;
	unsigned int index;
	t_xnor_expr *parent;
} t_xnor_expr_proxy;


void *xnor_expr_new(t_symbol *s, int argc, t_atom *argv)
{
  //create the driver and code visitor
  t_xnor_expr *x = NULL;

	if (strcmp("xnor/expr~", s->s_name) == 0) {
    x = (t_xnor_expr *)pd_new(xnor_expr_tilde_class);
    x->cpp = std::make_shared<cpp_expr>(XnorExpr::VECTOR);
  } else if (strcmp("xnor/fexpr~", s->s_name) == 0) {
    x = (t_xnor_expr *)pd_new(xnor_fexpr_tilde_class);
    x->cpp = std::make_shared<cpp_expr>(XnorExpr::SAMPLE);
	} else {
    if (strcmp("xnor/expr", s->s_name) != 0)
      error("xnor_expr_new: bad object name '%s'", s->s_name);
    x = (t_xnor_expr *)pd_new(xnor_expr_class);
    x->cpp = std::make_shared<cpp_expr>(XnorExpr::CONTROL);
	}

  //read in the arguments into a string
  char buf[1024];
  std::string line;
  for (int i = 0; i < argc; i++) {
    atom_string(&argv[i], buf, 1024);
    line += " " + std::string(buf);
  }
  
  //parse and setup
  try {
    auto statements = x->cpp->driver.parse_string(line);
    x->cpp->func = x->cpp->cv.function(statements, print_code);

    auto inputs = x->cpp->driver.inputs();

    //we have a minimum of 1 input, we might not use all these floats [in signal domain] but, whatever
    x->cpp->infloats.resize(std::max((size_t)1, inputs.size()), 0);
    x->cpp->inarg.resize(std::max((size_t)1, inputs.size()));
    x->cpp->input_types.resize(std::max((size_t)1, inputs.size()), xnor::ast::Variable::VarType::FLOAT);

    x->cpp->signal_inputs = 0;
    x->cpp->outarg.resize(statements.size());

    switch (x->cpp->expr_type) {
      case XnorExpr::CONTROL: 
        {
          if (inputs.size() >= 1 &&
              inputs.at(0)->type() != xnor::ast::Variable::VarType::FLOAT &&
              inputs.at(0)->type() != xnor::ast::Variable::VarType::INT) {
            error("the first inlet of xnor/expr must be a float or int");
            xnor_expr_free(x);
            return NULL;
          }

          //there will always be at least one output
          x->cpp->outfloat.resize(statements.size());

          for (size_t i = 0; i < x->cpp->outarg.size(); i++) {
            x->cpp->outarg[i] = &x->cpp->outfloat[i];
            x->cpp->outs.push_back(outlet_new(&x->x_obj, &s_float));
          }
        }
        break;
      case XnorExpr::VECTOR: 
        {
          if (inputs.size() >= 1 &&
              inputs.at(0)->type() != xnor::ast::Variable::VarType::VECTOR) {
            error("the first inlet of xnor/expr~ must be a vector");
            xnor_expr_free(x);
            return NULL;
          }

          for (size_t i = 0; i < statements.size(); i++)
            x->cpp->outs.push_back(outlet_new(&x->x_obj, &s_signal));
        }
        break;
      case XnorExpr::SAMPLE: 
        {
          if (inputs.size() >= 1 &&
              inputs.at(0)->type() != xnor::ast::Variable::VarType::INPUT) {
            error("the first inlet of xnor/fexpr~ must be a input sample variable");
            xnor_expr_free(x);
            return NULL;
          }

          for (size_t i = 0; i < statements.size(); i++) {
            x->cpp->outs.push_back(outlet_new(&x->x_obj, &s_signal));
            x->cpp->saved_outputs[i].resize(buffer_size, 0); //XXX saving a copy of the buffer, we may not actually need it though
          }
        }
        break;
    }

    for (size_t i = 0; i < inputs.size(); i++) {
      auto v = inputs.at(i);
      x->cpp->input_types[i] = v->type();
      switch (v->type()) {
        case xnor::ast::Variable::VarType::FLOAT:
        case xnor::ast::Variable::VarType::INT:
          if (i != 0) {
            t_xnor_expr_proxy *p = (t_xnor_expr_proxy *)pd_new(xnor_expr_proxy_class);
            p->index = v->input_index();
            p->parent = x;
            x->cpp->proxies.push_back(p);
            x->cpp->ins.push_back(inlet_new(&x->x_obj, &p->p_pd, &s_float, &s_float));
          }
          break;
        case xnor::ast::Variable::VarType::VECTOR:
          if (x->cpp->expr_type != XnorExpr::VECTOR) {
            error("cannot create vector inlet for xnor/expr or xnor/fexpr~");
            xnor_expr_free(x);
            return NULL;
          }
          if (i != 0)
            x->cpp->ins.push_back(inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal));
          x->cpp->signal_inputs += 1;
          break;
        case xnor::ast::Variable::VarType::INPUT:
          if (x->cpp->expr_type != XnorExpr::SAMPLE) {
            error("input sample inlet only works for xnor/fexpr~");
            xnor_expr_free(x);
            return NULL;
          }
          if (i != 0)
            x->cpp->ins.push_back(inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal));
          x->cpp->signal_inputs += 1;
          x->cpp->saved_inputs[v->input_index()].resize(buffer_size * 2, 0); //input buffers need access to last input as well
          break;
        default:
          throw std::runtime_error("input type not handled " + std::to_string(v->input_index()));
      }
    }
  } catch (std::runtime_error& e) {
    error("error parsing \"%s\" %s", line.c_str(), e.what());
    xnor_expr_free(x);
    return NULL;
  }

  return (void *)x;
}

void xnor_expr_free(t_xnor_expr * x) {
  if (x == NULL)
    return;
  x->cpp = nullptr;
}

void xnor_expr_bang(t_xnor_expr * x) {
  //assign input values
  for (size_t i = 0; i < x->cpp->inarg.size(); i++)
    x->cpp->inarg.at(i).flt = x->cpp->infloats.at(i);

  //execute function
  x->cpp->func(&x->cpp->outarg.front(), &x->cpp->inarg.front(), 1);

  //output!
  for (unsigned int i = 0; i < x->cpp->outarg.size(); i++)
    outlet_float(x->cpp->outs.at(i), *(x->cpp->outarg.at(i)));
}

static void xnor_expr_list(t_xnor_expr *x, t_symbol * /*s*/, int argc, const t_atom *argv) {
  for (int i = 0; i < std::min(argc, (int)x->cpp->infloats.size()); i++) {
		if (argv[i].a_type == A_FLOAT) {
      x->cpp->infloats.at(i) = argv[i].a_w.w_float;
		} else {
			pd_error(x, "expr: type mismatch");
		}
  }
  xnor_expr_bang(x);
}

void xnor_expr_proxy_float(t_xnor_expr_proxy *p, t_floatarg f) {
  //post("%d got float %f", p->index, f);
  p->parent->cpp->infloats.at(p->index) = f;
}


static t_int *xnor_expr_tilde_perform(t_int *w) {
  t_xnor_expr *x = (t_xnor_expr *)(w[1]);
  int n = std::min((int)(w[2]), buffer_size); //XXX how do we figure out the real buffer size?

  int vector_index = 3;
  for (unsigned int i = 0; i < x->cpp->input_types.size(); i++) {
    switch (x->cpp->input_types.at(i)) {
        case xnor::ast::Variable::VarType::FLOAT:
        case xnor::ast::Variable::VarType::INT:
          x->cpp->inarg.at(i).flt = x->cpp->infloats.at(i);
          break;
        case xnor::ast::Variable::VarType::VECTOR:
          x->cpp->inarg.at(i).vec = (t_sample*)w[vector_index++];
          break;
        case xnor::ast::Variable::VarType::INPUT:
          {
            float * in = (t_sample*)w[vector_index++];
            float * buf = &x->cpp->saved_inputs.at(i).front();
            memcpy(buf + n, buf, n * sizeof(float)); //copy the old data forward
            memcpy(buf, in, n * sizeof(float)); //copy the new data in
            x->cpp->inarg.at(i).vec = buf;
          }
          break;
        default:
          //XXX
          break;
    }
  }

  if (x->cpp->expr_type == XnorExpr::SAMPLE) {
    //render to the saved buffers [which has some old needed data into it]
    for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
      x->cpp->outarg.at(i) = &x->cpp->saved_outputs.at(i).front();
    }
    x->cpp->func(&x->cpp->outarg.front(), &x->cpp->inarg.front(), n);

    //copy out the saved buffers
    for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
      auto f = (float *)w[vector_index++];
      memcpy(f, &x->cpp->saved_outputs.at(i).front(), sizeof(float) * n);
    }
  } else {
    for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
      x->cpp->outarg.at(i) = (float *)w[vector_index++];
    }
    x->cpp->func(&x->cpp->outarg.front(), &x->cpp->inarg.front(), n);
  }
  return w + vector_index;
}

static void xnor_expr_tilde_dsp(t_xnor_expr *x, t_signal **sp)
{
  int input_signals = x->cpp->signal_inputs;
  int output_signals = x->cpp->outarg.size();

  int vecsize = input_signals + output_signals + 2;
  t_int ** vec = (t_int **)getbytes(sizeof(t_int) * vecsize);
  vec[0] = (t_int*)x;
  vec[1] = (t_int*)sp[0]->s_n;

  //add the inputs
  int voffset = 2;
  for (int i = 0; i < input_signals; i++)
    vec[i + voffset] = (t_int*)sp[i]->s_vec;

  //then the outputs
  voffset += input_signals;
  for (int i = 0; i < output_signals; i++)
    vec[i + voffset] = (t_int*)sp[i + input_signals]->s_vec;
  dsp_addv(xnor_expr_tilde_perform, vecsize, (t_int*)vec);
  freebytes(vec, sizeof(t_int) * vecsize);
}

void xnor_expr_setup(void) {
  xnor::LLVMCodeGenVisitor::init();

  xnor_expr_class = class_new(gensym("xnor/expr"),
      (t_newmethod)xnor_expr_new,
      (t_method)xnor_expr_free,
      sizeof(t_xnor_expr),
      0,
      A_GIMME, 0);

  class_addlist(xnor_expr_class, xnor_expr_list);
  class_addbang(xnor_expr_class, xnor_expr_bang);

	xnor_expr_proxy_class = class_new(gensym("xnor_expr_proxy"),
      0, 0,
      sizeof(t_xnor_expr_proxy),
      CLASS_PD,
      A_NULL);
	class_addfloat(xnor_expr_proxy_class, xnor_expr_proxy_float);

  xnor_expr_tilde_class = class_new(gensym("xnor/expr~"),
      (t_newmethod)xnor_expr_new,
      (t_method)xnor_expr_free,
      sizeof(t_xnor_expr),
      0,
      A_GIMME, 0);
	class_addmethod(xnor_expr_tilde_class, nullfn, gensym("signal"), A_NULL);
	CLASS_MAINSIGNALIN(xnor_expr_tilde_class, t_xnor_expr, exp_f);
	class_addmethod(xnor_expr_tilde_class, (t_method)xnor_expr_tilde_dsp, gensym("dsp"), A_NULL);

  xnor_fexpr_tilde_class = class_new(gensym("xnor/fexpr~"),
      (t_newmethod)xnor_expr_new,
      (t_method)xnor_expr_free,
      sizeof(t_xnor_expr),
      0,
      A_GIMME, 0);
	class_addmethod(xnor_fexpr_tilde_class, nullfn, gensym("signal"), A_NULL);
	CLASS_MAINSIGNALIN(xnor_fexpr_tilde_class, t_xnor_expr, exp_f);
	class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_expr_tilde_dsp, gensym("dsp"), A_NULL);
}

//utility functions

namespace {
  int facti(int i) {
    if (i <= 0)
      return 1;
    return i * facti(i - 1);
  }
}

float xnor_expr_factf(float v) {
  return static_cast<float>(facti(static_cast<int>(v)));
}

//adapted from max_ex_tab x_vexpr_if.c
float * xnor_expr_table_value_ptr(t_symbol * name, float findex) {
  t_garray * a;
  int size = 0;
  t_word *vec;
  if (!name || !(a = (t_garray *)pd_findbyclass(name, garray_class)) || !garray_getfloatwords(a, &size, &vec)) {
    //XXX post error
    return nullptr;
  }

  int index = std::min(std::max(0, static_cast<int>(findex)), size - 1);
  return &(vec[index].w_float);
}

float xnor_expr_maxf(float a, float b) { return std::max(a, b); }

float xnor_expr_value_assign(t_symbol * name, float v) {
  value_setfloat(name, v);
  return v;
}

float xnor_expr_value_get(t_symbol * name) {
  float v = 0;
  return value_getfloat(name, &v) == 0 ? v : 0;
}

