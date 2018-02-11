#include <m_pd.h>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <algorithm>
#include <random>
#include "llvmcodegen/codegen.h"
#include "parser.hh"

struct _xnor_expr_proxy;
struct _xnor_expr;

namespace {
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
    std::vector<t_symbol *> symbol_inputs;
    std::map<unsigned int, std::vector<t_sample>> saved_inputs;
    std::map<unsigned int, std::vector<t_sample>> saved_outputs;

    std::vector<xnor::LLVMCodeGenVisitor::input_arg_t> inarg;
    std::vector<xnor::ast::Variable::VarType> input_types;
    int signal_inputs = 0; //could just calc from input_types

    bool compute = true;
    std::string code_printout;

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
extern "C" void xnor_expr_start(struct _xnor_expr * x);
extern "C" void xnor_expr_stop(struct _xnor_expr * x);
extern "C" void xnor_expr_print(struct _xnor_expr * x);
extern "C" void xnor_expr_setup(void);
extern "C" void xnor_fexpr_tilde_set(struct _xnor_expr *x, t_symbol *s, int argc, t_atom *argv);
extern "C" void xnor_fexpr_tilde_clear(struct _xnor_expr *x, t_symbol *s, int argc, t_atom *argv);

//functions called from generated code
extern "C" float xnor_expr_fact(float v);
extern "C" float * xnor_expr_table_value_ptr(t_symbol * name, float findex);
extern "C" float xnor_expr_table_size(t_symbol * name);
extern "C" float xnor_expr_table_sum(t_symbol * name, float start, float end);
extern "C" float xnor_expr_table_sum_all(t_symbol * name);
extern "C" float xnor_expr_max(float a, float b);
extern "C" float xnor_expr_min(float a, float b);
extern "C" float xnor_expr_random(float a, float b);
extern "C" float xnor_expr_imodf(float v);
extern "C" float xnor_expr_modf(float v);

extern "C" float xnor_expr_isnan(float v);
extern "C" float xnor_expr_isinf(float v);
extern "C" float xnor_expr_finite(float v);

extern "C" float xnor_expr_value_assign(t_symbol * name, float v);
extern "C" float xnor_expr_value_get(t_symbol * name);
extern "C" float xnor_expr_deref(float * v);

extern "C" float xnor_expr_array_read(float * array, float index, int array_length);

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
    //make sure there is more than just a space
    if (line.find_first_not_of(' ') == std::string::npos) {
      x->cpp->func = nullptr;
    } else {
      auto statements = x->cpp->driver.parse_string(line);
      x->cpp->func = x->cpp->cv.function(statements, x->cpp->code_printout);

      auto inputs = x->cpp->driver.inputs();

      //we have a minimum of 1 input, we might not use all these floats [in signal domain] but, whatever
      x->cpp->infloats.resize(std::max((size_t)1, inputs.size()), 0);
      x->cpp->symbol_inputs.resize(std::max((size_t)1, inputs.size()), nullptr);
      x->cpp->inarg.resize(std::max((size_t)1, inputs.size()));
      x->cpp->input_types.resize(std::max((size_t)1, inputs.size()), xnor::ast::Variable::VarType::FLOAT);

      x->cpp->signal_inputs = 0;
      x->cpp->outarg.resize(statements.size());

      switch (x->cpp->expr_type) {
        case XnorExpr::CONTROL: 
          {
            if (inputs.size() >= 1 &&
                inputs.at(0)->type() != xnor::ast::Variable::VarType::FLOAT &&
                inputs.at(0)->type() != xnor::ast::Variable::VarType::INT &&
                inputs.at(0)->type() != xnor::ast::Variable::VarType::SYMBOL) {
              error("the first inlet of xnor/expr must be a float, int or symbol");
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
          case xnor::ast::Variable::VarType::SYMBOL:
            if (i != 0)
              x->cpp->ins.push_back(symbolinlet_new(&x->x_obj, &x->cpp->symbol_inputs.at(i)));
            break;
          default:
            throw std::runtime_error("input type not handled " + std::to_string(v->input_index()));
        }
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
  if (x->cpp->func == nullptr)
    return;

  //assign input values
  for (size_t i = 0; i < x->cpp->inarg.size(); i++) {
    auto t = x->cpp->input_types.at(i);
    switch (t) {
      case xnor::ast::Variable::VarType::FLOAT:
      case xnor::ast::Variable::VarType::INT:
        x->cpp->inarg.at(i).flt = x->cpp->infloats.at(i);
        break;
      case xnor::ast::Variable::VarType::SYMBOL:
        x->cpp->inarg.at(i).sym = x->cpp->symbol_inputs.at(i);
        break;
      default:
        error("unhandled input type");
        break;
    }
  }

  //execute function
  x->cpp->func(&x->cpp->outarg.front(), &x->cpp->inarg.front(), 1);

  //output!
  for (unsigned int i = 0; i < x->cpp->outarg.size(); i++)
    outlet_float(x->cpp->outs.at(i), *(x->cpp->outarg.at(i)));
}

static void xnor_expr_list(t_xnor_expr *x, t_symbol * /*s*/, int argc, const t_atom *argv) {
  for (int i = 0; i < std::min(argc, (int)x->cpp->infloats.size()); i++) {
    auto t = x->cpp->input_types.at(i);
		if (argv[i].a_type == A_FLOAT && (t == xnor::ast::Variable::VarType::FLOAT || t == xnor::ast::Variable::VarType::INT)) {
      x->cpp->infloats.at(i) = argv[i].a_w.w_float;
    } else if (argv[i].a_type == A_SYMBOL && t == xnor::ast::Variable::VarType::SYMBOL) {
      x->cpp->symbol_inputs.at(i) = argv[i].a_w.w_symbol;
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
        case xnor::ast::Variable::VarType::SYMBOL:
          x->cpp->inarg.at(i).sym = x->cpp->symbol_inputs.at(i);
          break;
        case xnor::ast::Variable::VarType::VECTOR:
          x->cpp->inarg.at(i).vec = (t_sample*)w[vector_index++];
          break;
        case xnor::ast::Variable::VarType::INPUT:
          {
            t_sample * in = (t_sample*)w[vector_index++];
            t_sample * buf = &x->cpp->saved_inputs.at(i).front();
            memcpy(buf + n, buf, n * sizeof(t_sample)); //copy the old data forward
            memcpy(buf, in, n * sizeof(t_sample)); //copy the new data in
            x->cpp->inarg.at(i).vec = buf;
          }
          break;
        default:
          //XXX
          break;
    }
  }

  //if we're not computing then we just clear everything out
  if (!x->cpp->compute) {
    size_t vsize = x->cpp->saved_outputs.size() ? x->cpp->saved_outputs.begin()->second.size() : 0;
    for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
      auto p = (t_sample *)w[vector_index++];
      memset(p, 0, vsize * sizeof(t_sample));
    }
  } else {
    if (x->cpp->expr_type == XnorExpr::SAMPLE) {
      //render to the saved buffers [which has some old needed data into it]
      for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
        x->cpp->outarg.at(i) = &x->cpp->saved_outputs.at(i).front();
      }
      x->cpp->func(&x->cpp->outarg.front(), &x->cpp->inarg.front(), n);

      //copy out the saved buffers
      for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
        auto f = (t_sample *)w[vector_index++];
        memcpy(f, &x->cpp->saved_outputs.at(i).front(), sizeof(t_sample) * n);
      }
    } else {
      for (unsigned int i = 0; i < x->cpp->outarg.size(); i++) {
        x->cpp->outarg.at(i) = (t_sample *)w[vector_index++];
      }
      x->cpp->func(&x->cpp->outarg.front(), &x->cpp->inarg.front(), n);
    }
  }
  return w + vector_index;
}

static void xnor_expr_tilde_dsp(t_xnor_expr *x, t_signal **sp)
{
  if (x->cpp->func == nullptr)
    return;

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

void xnor_fexpr_set_usage() {
  post("xnor/fexpr~: set val ...");
  post("xnor/fexpr~: set {xy}[#] val ...");
}

void xnor_fexpr_clear_usage() {
  post("xnor/fexpr~ usage: 'clear' or 'clear {xy}[#]'");
}

// taken directly from x_vexpr_if.c and modified
void xnor_fexpr_tilde_set(t_xnor_expr *x, t_symbol * /*s*/, int argc, t_atom *argv) {
  t_symbol *sx;
  int vecno, vsize, nargs;

  if (!argc)
    return;
  sx = atom_getsymbolarg(0, argc, argv);
  switch(sx->s_name[0]) {
    case 'x': {
        if (!sx->s_name[1])
          vecno = 0;
        else {
          vecno = atoi(sx->s_name + 1);
          if (vecno <= 0) {
            post("xnor/fexpr~ set: bad set x vector number");
            xnor_fexpr_set_usage();
            return;
          }
          vecno--;
        }
        auto it = x->cpp->saved_inputs.find(vecno);
        if (it == x->cpp->saved_inputs.end()) {
          post("xnor/fexpr~ set: no signal at inlet %d", vecno + 1);
          return;
        }
        nargs = argc - 1;
        if (nargs <= 0) {
          post("xnor/fexpr~ set: no argument to set");
          return;
        }
        vsize = it->second.size();
        if (nargs > vsize) {
          post("xnor/fexpr~ set: %d set values larger than vector size(%d)", nargs, vsize);
          post("xnor/fexpr~ set: only the first %d values will be set", vsize);
          nargs = vsize;
        }
        for (int i = 0; i < nargs; i++) {
          it->second.at(vsize - i - 1) = atom_getfloatarg(i + 1, argc, argv);
        }
      }
      return;
    case 'y': {
        if (!sx->s_name[1])
          vecno = 0;
        else {
          vecno = atoi(sx->s_name + 1);
          if (vecno <= 0) {
            post("xnor/fexpr~ set: bad set y vector number");
            xnor_fexpr_set_usage();
            return;
          }
          vecno--;
        }
        auto it = x->cpp->saved_outputs.find(vecno);
        if (it == x->cpp->saved_outputs.end()) {
          post("xnor/fexpr~ set: outlet out of range");
          return;
        }
        nargs = argc - 1;
        if (nargs <= 0) {
          post("xnor/fexpr~ set: no argument to set");
          return;
        }
        vsize = it->second.size();
        if (nargs > vsize) {
          post("xnor/fexpr~ set: %d set values larger than vector size(%d)", nargs, vsize);
          post("xnor/fexpr~ set: only the first %d values will be set", vsize);
          nargs = vsize;
        }
        for (int i = 0; i < nargs; i++) {
          it->second.at(vsize - i - 1) = atom_getfloatarg(i + 1, argc, argv);
        }
      }
      return;
    case 0: {
        int nouts = x->cpp->saved_outputs.size();
        if (argc > nouts) {
          post("xnor/fexpr~ set: only %d outlets available", nouts);
          post("xnor/fexpr~ set: the extra set values are ignored");
        }
        for (int i = 0; i < nouts && i < argc; i++) {
          auto it = x->cpp->saved_outputs.find(i);
          if (it == x->cpp->saved_outputs.end())
            continue;
          it->second.at(it->second.size() - 1) = atom_getfloatarg(i, argc, argv);
        }
      }
      return;
    default:
      xnor_fexpr_set_usage();
      return;
  }
  return;
}

// taken directly from x_vexpr_if.c and modified
void xnor_fexpr_tilde_clear(t_xnor_expr *x, t_symbol * /*s */, int argc, t_atom *argv) {
  t_symbol *sx;
  int vecno;

  /*
   *  if no argument clear all input and output buffers
   */
  if (argc <= 0) {
    for (auto& it: x->cpp->saved_inputs) {
      memset(&it.second.front(), 0, it.second.size() * sizeof(t_sample));
    }
    for (auto& it: x->cpp->saved_outputs) {
      memset(&it.second.front(), 0, it.second.size() * sizeof(t_sample));
    }
    return;
  }
  if (argc > 1) {
    xnor_fexpr_clear_usage();
    return;
  }

  sx = atom_getsymbolarg(0, argc, argv);
  switch(sx->s_name[0]) {
    case 'x':
      if (!sx->s_name[1])
        vecno = 0;
      else {
        vecno = atoi(sx->s_name + 1);
        if (vecno <= 0) {
          post("xnor/fexpr~ clear: bad clear x vector number");
          return;
        }
        vecno--;
      }
      {
        auto it = x->cpp->saved_inputs.find(vecno);
        if (it == x->cpp->saved_inputs.end()) {
          post("xnor/fexpr~ clear: no signal at inlet %d", vecno + 1);
          return;
        }
        memset(&it->second.front(), 0, it->second.size() * sizeof(t_sample));
      }
      return;
    case 'y':
      if (!sx->s_name[1])
        vecno = 0;
      else {
        vecno = atoi(sx->s_name + 1);
        if (vecno <= 0) {
          post("xnor/fexpr~ clear: bad clear y vector number");
          return;
        }
        vecno--;
      }
      {
        auto it = x->cpp->saved_outputs.find(vecno);
        if (it == x->cpp->saved_outputs.end()) {
          post("xnor/fexpr~ clear: no signal at outlet %d", vecno + 1);
          return;
        }
        memset(&it->second.front(), 0, it->second.size() * sizeof(t_sample));
      }
      return;
    default:
      xnor_fexpr_clear_usage();
      return;
  }
}

void xnor_expr_start(t_xnor_expr *x) { x->cpp->compute = true; }
void xnor_expr_stop(t_xnor_expr *x) { x->cpp->compute = false; }
void xnor_expr_print(t_xnor_expr *x) {
  switch (x->cpp->expr_type) {
    case XnorExpr::CONTROL: 
      post("xnor/expr: ");
      break;
    case XnorExpr::VECTOR: 
      post("xnor/expr~: ");
      break;
    case XnorExpr::SAMPLE: 
      post("xnor/fexpr~: ");
      break;
  }
  std::stringstream ss(x->cpp->code_printout.c_str());
  std::string out;
  while (std::getline(ss, out)) {
    poststring(out.c_str());
    poststring("\n");
  }
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
  class_addmethod(xnor_expr_class, (t_method)xnor_expr_print, gensym("print"), A_NULL);
  class_sethelpsymbol(xnor_expr_class, gensym("xnor-expr"));

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
  class_addmethod(xnor_expr_tilde_class, (t_method)xnor_expr_print, gensym("print"), A_NULL);
  class_sethelpsymbol(xnor_expr_tilde_class, gensym("xnor-expr"));

  xnor_fexpr_tilde_class = class_new(gensym("xnor/fexpr~"),
      (t_newmethod)xnor_expr_new,
      (t_method)xnor_expr_free,
      sizeof(t_xnor_expr),
      0,
      A_GIMME, 0);
	class_addmethod(xnor_fexpr_tilde_class, nullfn, gensym("signal"), A_NULL);
	CLASS_MAINSIGNALIN(xnor_fexpr_tilde_class, t_xnor_expr, exp_f);
	class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_expr_tilde_dsp, gensym("dsp"), A_NULL);
  class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_fexpr_tilde_set, gensym("set"), A_GIMME, 0);
  class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_fexpr_tilde_clear, gensym("clear"), A_GIMME, 0);
  class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_expr_start, gensym("start"), A_NULL);
  class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_expr_stop, gensym("stop"), A_NULL);
  class_addmethod(xnor_fexpr_tilde_class, (t_method)xnor_expr_print, gensym("print"), A_NULL);
  class_sethelpsymbol(xnor_fexpr_tilde_class, gensym("xnor-expr"));
}

//utility functions

namespace {
  int facti(int i) {
    if (i <= 0)
      return 1;
    return i * facti(i - 1);
  }

  //adapted from max_ex_tab x_vexpr_if.c
  t_word * xnor_get_table(t_symbol *name, int& sizeout) {
    t_garray * a;
    sizeout = 0;
    t_word *vec;
    if (!name || !(a = (t_garray *)pd_findbyclass(name, garray_class)) || !garray_getfloatwords(a, &sizeout, &vec)) {
      sizeout = 0; //in case it was altered?
      //XXX post error
      return nullptr;
    }
    return vec;
  }

  //if end < 0, end == size
  float xnor_expr_table_sum_range(t_symbol * name, ssize_t start, ssize_t end) {
    int s = 0;
    t_word * vec = xnor_get_table(name, s);
    if (!vec)
      return 0.0f;

    ssize_t size = s;

    start = std::min(std::max(start, static_cast<ssize_t>(0)), size);
    if (end < 0)
      end = size;
    else
      end = std::min(std::max(end, static_cast<ssize_t>(0)), size);

    float sum = 0;
    for (ssize_t i = start; i < end; i++)
      sum += vec[i].w_float;
    return sum;
  }
}

float xnor_expr_fact(float v) {
  return static_cast<float>(facti(static_cast<int>(v)));
}

float * xnor_expr_table_value_ptr(t_symbol * name, float findex) {
  if (!name)
    return nullptr;

  int size = 0;
  t_word * vec = xnor_get_table(name, size);
  if (!vec || size <= 0) {
    return nullptr;
  }
  int index = std::min(std::max(0, static_cast<int>(findex)), size - 1);
  return &(vec[index].w_float);
}

float xnor_expr_table_size(t_symbol * name) {
  int size = 0;
  xnor_get_table(name, size);
  return static_cast<float>(size);
}

float xnor_expr_table_sum(t_symbol * name, float fstart, float fend) {
  if (fstart > fend || fend < 0)
    return 0.0f;
  return xnor_expr_table_sum_range(name, static_cast<ssize_t>(fstart), static_cast<ssize_t>(fend) + 1);
}

float xnor_expr_table_sum_all(t_symbol * name) {
  return xnor_expr_table_sum_range(name, 0, -1);
}

float xnor_expr_max(float a, float b) { return std::max(a, b); }
float xnor_expr_min(float a, float b) { return std::min(a, b); }
float xnor_expr_random(float fstart, float fend) {
  int start = static_cast<int>(fstart);
  int end = static_cast<int>(fend - 1);
  if (start >= end)
    return 0;

  //https://stackoverflow.com/questions/21237905/how-do-i-generate-thread-safe-uniform-random-numbers
  static thread_local std::mt19937 generator;
  std::uniform_int_distribution<int> distribution(start, end);
  return static_cast<float>(distribution(generator));
}

float xnor_expr_imodf(float v) {
  return truncf(v);
}

float xnor_expr_modf(float v) {
  return v - truncf(v);
}

float xnor_expr_isnan(float v) { return isnanf(v) ? 1 : 0; }
float xnor_expr_isinf(float v) { return isinff(v) ? 1 : 0; }
float xnor_expr_finite(float v) { return finitef(v) ? 1 : 0; }

float xnor_expr_value_assign(t_symbol * name, float v) {
  if (name)
    value_setfloat(name, v);
  return v;
}

float xnor_expr_value_get(t_symbol * name) {
  float v = 0;
  return (name && value_getfloat(name, &v) == 0) ? v : 0;
}

float xnor_expr_deref(float * v) {
  return v != 0 ? *v : 0;
}

float xnor_expr_array_read(float * array, float index, int array_length) {
  int i = static_cast<int>(index);
  float off = index - static_cast<float>(i);
  float v1 = array[i % array_length];
  float v2 = array[(i + 1) % array_length];
  return v2 * off  + v1 * (1.0 - off);
}

