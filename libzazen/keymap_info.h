#ifndef LIBZAZEN_KEYMAP_INFO_H
#define LIBZAZEN_KEYMAP_INFO_H

#include <xkbcommon/xkbcommon.h>
#include <z11-server-protocol.h>

struct zazen_keymap_info {
  struct xkb_context *context;
  struct xkb_rule_names *names;
  struct xkb_keymap *keymap;

  int fd;
  size_t size;
  enum z11_keyboard_keymap_format format;
};

struct zazen_keymap_info *zazen_keymap_info_create();

void zazen_keymap_info_destroy(struct zazen_keymap_info *keymap_info);

#endif  // LIBZAZEN_KEYMAP_INFO_H
