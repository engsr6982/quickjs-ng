/* Modified to load bytecode from file instead of using embedded array */
#include "quickjs-libc.h"
#include <stdio.h>
#include <stdlib.h>

static JSContext *JS_NewCustomContext(JSRuntime *rt) {
  JSContext *ctx = JS_NewContext(rt);
  if (!ctx)
    return NULL;
  return ctx;
}

static uint8_t *load_bytecode_file(const char *filename, size_t *out_size) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Could not open bytecode file: %s\n", filename);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *buf = malloc(size);
  if (!buf) {
    fclose(f);
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  if (fread(buf, 1, size, f) != (size_t)size) {
    fclose(f);
    free(buf);
    fprintf(stderr, "Failed to read bytecode file\n");
    return NULL;
  }

  fclose(f);
  *out_size = (size_t)size;
  return buf;
}

int main(int argc, char **argv) {
  int r = 0;
  JSRuntime *rt;
  JSContext *ctx;
  size_t bytecode_size;
  uint8_t *bytecode_buf;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <bytecode_file.jsb>\n", argv[0]);
    return 1;
  }

  bytecode_buf = load_bytecode_file(argv[1], &bytecode_size);
  if (!bytecode_buf) {
    return 1;
  }

  rt = JS_NewRuntime();
  if (!rt) {
    free(bytecode_buf);
    fprintf(stderr, "Failed to create JS runtime\n");
    return 1;
  }

  js_std_set_worker_new_context_func(JS_NewCustomContext);
  js_std_init_handlers(rt);
  JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

  ctx = JS_NewCustomContext(rt);
  if (!ctx) {
    free(bytecode_buf);
    JS_FreeRuntime(rt);
    fprintf(stderr, "Failed to create JS context\n");
    return 1;
  }

  js_std_add_helpers(ctx, argc, argv);

  js_std_eval_binary(ctx, bytecode_buf, bytecode_size, 0);

  r = js_std_loop(ctx);
  if (r) {
    js_std_dump_error(ctx);
  }

  free(bytecode_buf);
  js_std_free_handlers(rt);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return r;
}
