#include <m_pd.h>
#include <string>
#include "llvmcodegen/codegen.h"
#include "parser.hh"
#include <stdexcept>
#include <vector>
#include <memory>

extern "C" void *xnor_expr_new(t_symbol *s, int argc, t_atom *argv);
extern "C" void xnor_expr_setup(void);

static t_class *xnor_expr_class;
static t_class *xnor_expr_proxy_class;

struct _xnor_expr_proxy;

typedef struct _xnor_expr {
  t_object x_obj;
  std::vector<t_inlet *> ins;
  std::vector<t_outlet *> outs;
  std::vector<struct _xnor_expr_proxy *> proxies;

  std::shared_ptr<parse::Driver> driver;
  std::shared_ptr<xnor::LLVMCodeGenVisitor> cv;

  xnor::LLVMCodeGenVisitor::function_t func;

  std::vector<float> outfloat;
  std::vector<float *> outarg;
  std::vector<float> infloats;
  std::vector<xnor::LLVMCodeGenVisitor::input_arg_t> inarg;
} t_xnor_expr;

typedef struct _xnor_expr_proxy {
	t_pd p_pd;
	unsigned int index;
	t_xnor_expr *parent;
} t_xnor_expr_proxy;

void *xnor_expr_new(t_symbol *s, int argc, t_atom *argv)
{
  t_xnor_expr *x = (t_xnor_expr *)pd_new(xnor_expr_class);
  x->driver = std::make_shared<parse::Driver>();
  x->cv = std::make_shared<xnor::LLVMCodeGenVisitor>();

  std::string line;

  char buf[1024];
  for (int i = 0; i < argc; i++) {
    atom_string(&argv[i], buf, 1024);
    line += " " + std::string(buf);
  }
  
  try {
    auto t = x->driver->parse_string(line);
    x->func = x->cv->function(t);

    auto inputs = x->driver->inputs();
    x->infloats.resize(inputs.size(), 0);
    x->inarg.resize(inputs.size());

    x->outfloat.resize(t.size());
    x->outarg.resize(t.size());

    for (size_t i = 0; i < x->outarg.size(); i++)
      x->outarg[i] = &x->outfloat[i];

    if (inputs.size() == 0) {
      x->ins.push_back(inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_bang, &s_bang));
    } else {
      for (auto i: inputs) {
        switch (i->type()) {
          case xnor::ast::Variable::VarType::FLOAT:
          case xnor::ast::Variable::VarType::INT:
            {
              t_xnor_expr_proxy *p = (t_xnor_expr_proxy *)pd_new(xnor_expr_proxy_class);
              p->index = i->input_index();
              p->parent = x;
              x->proxies.push_back(p);

              x->ins.push_back(inlet_new(&x->x_obj, &p->p_pd, &s_float, &s_float));
            }
            break;
          default:
            throw std::runtime_error("input type not handled yet");
        }
      }
    }
    for (auto c: t) {
      x->outs.push_back(outlet_new(&x->x_obj, &s_float));
    }
  } catch (std::runtime_error& e) {
    error("error parsing \"%s\" %s", line.c_str(), e.what());
    x->driver = nullptr;
    return NULL;
  }

  return (void *)x;
}

void xnor_expr_free(t_xnor_expr * x) {
  if (x == NULL)
    return;

  x->driver = nullptr;
  x->cv = nullptr;

  for (auto i: x->ins)
    inlet_free(i);
  x->ins.clear();

  for (auto i: x->outs)
    outlet_free(i);
  x->outs.clear();
}

void xnor_expr_bang(t_xnor_expr * x) {
  //assign input values
  for (size_t i = 0; i < x->inarg.size(); i++)
    x->inarg.at(i).flt = x->infloats.at(i);

  //execute function
  x->func(&x->outarg.front(), &x->inarg.front());

  //output!
  for (unsigned int i = 0; i < x->outarg.size(); i++)
    outlet_float(x->outs.at(i), *(x->outarg.at(i)));
}

static void xnor_expr_list(t_xnor_expr *x, t_symbol *s, int argc, const t_atom *argv) {
  for (int i = 0; i < std::min(argc, (int)x->infloats.size()); i++) {
		if (argv[i].a_type == A_FLOAT) {
      x->infloats.at(i) = argv[i].a_w.w_float;
		} else {
			pd_error(x, "expr: type mismatch");
		}
  }
  xnor_expr_bang(x);
}

void xnor_expr_proxy_float(t_xnor_expr_proxy *p, t_floatarg f) {
  post("%d got float %f", p->index, f);
  p->parent->infloats.at(p->index) = f;
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

  /*
  class_addmethod(xnor_expr_class,
      (t_method)xnor_expr_reset, gensym("reset"), 0);
  class_addmethod(xnor_expr_class,
      (t_method)xnor_expr_set, gensym("set"),
      A_DEFFLOAT, 0);
  class_addmethod(xnor_expr_class,
      (t_method)xnor_expr_bound, gensym("bound"),
      A_DEFFLOAT, A_DEFFLOAT, 0);
  class_sethelpsymbol(xnor_expr_class, gensym("help-xnor_expr"));
      */
}
