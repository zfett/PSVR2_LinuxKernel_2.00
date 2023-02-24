#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>

struct dentry *mtk_debugfs_root;
EXPORT_SYMBOL(mtk_debugfs_root);

static int __init mtk_debugfs_init(void)
{
	mtk_debugfs_root = debugfs_create_dir("mediatek", NULL);
	if (!mtk_debugfs_root)
		return -ENOMEM;

	return 0;
}
arch_initcall(mtk_debugfs_init);
