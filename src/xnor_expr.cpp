#include <m_pd.h>
#include "llvmcodegen/codegen.h"
#include "parser.hh"

extern "C" void *xnor_expr_new(t_symbol *s, int argc, t_atom *argv);
extern "C" void xnor_expr_setup(void);

static t_class *xnor_expr_class;

typedef struct _xnor_expr {
  t_object x_obj;
  t_inlet **ins;
  t_outlet **outs;
} t_xnor_expr;

void *xnor_expr_new(t_symbol *s, int argc, t_atom *argv)
{
  t_xnor_expr *x = (t_xnor_expr *)pd_new(xnor_expr_class);
  //inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("bound"));

  //floatinlet_new(&x->x_obj, &x->step);
  //x->f_out = outlet_new(&x->x_obj, &s_float);
  //x->b_out = outlet_new(&x->x_obj, &s_bang);
  return (void *)x;
}

void xnor_expr_setup(void) {
  xnor_expr_class = class_new(gensym("xnor_expr"),
      (t_newmethod)xnor_expr_new,
      0, sizeof(t_xnor_expr),
      CLASS_DEFAULT,
      A_GIMME, 0);

  /*
  class_addbang (xnor_expr_class, xnor_expr_bang);
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
