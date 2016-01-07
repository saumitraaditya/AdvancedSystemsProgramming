#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x5eadf54a, "module_layout" },
	{ 0xf62d269d, "class_destroy" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x21a21ea4, "device_destroy" },
	{ 0x37a0cba, "kfree" },
	{ 0xb714e933, "cdev_del" },
	{ 0xf1faf509, "device_create" },
	{ 0x9b604f4c, "cdev_add" },
	{ 0x48eb0c0d, "__init_waitqueue_head" },
	{ 0xe41b8ba6, "cdev_init" },
	{ 0x86a4889a, "kmalloc_order_trace" },
	{ 0x83699014, "kmem_cache_alloc_trace" },
	{ 0x85e90336, "kmalloc_caches" },
	{ 0xd6e17aa6, "__class_create" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xe45f60d8, "__wake_up" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0xdd1a2871, "down" },
	{ 0x75bb675a, "finish_wait" },
	{ 0x622fa02a, "prepare_to_wait" },
	{ 0x4292364c, "schedule" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x131ff057, "current_task" },
	{ 0x50eedeb8, "printk" },
	{ 0xc4554217, "up" },
	{ 0x4792c572, "down_interruptible" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "99E79E276F991180142933C");
