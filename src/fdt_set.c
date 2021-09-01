#include "libfdt/libfdt.h"
#include "fdt.h"

#define DTB_PAD_SIZE 1024

extern void *fdt;

static rt_off_t fdt_find_and_add_subnode(char* name)
{
    rt_off_t chosen_offset = 0;

    chosen_offset = fdt_subnode_offset(fdt, 0, name);

    if (chosen_offset == -FDT_ERR_NOTFOUND)
    {
        chosen_offset = fdt_add_subnode(fdt, 0, name);
    }

    return chosen_offset;
}

rt_size_t fdt_set_linux_cmdline(char *cmdline)
{
    rt_off_t chosen_offset;
    rt_size_t cmdline_size;

    if (cmdline == RT_NULL || fdt == RT_NULL)
    {
        goto end;
    }

    chosen_offset = fdt_find_and_add_subnode("chosen");
    cmdline_size = rt_strlen(cmdline);

    /* install bootargs */
    if (chosen_offset >= 0 || chosen_offset == -FDT_ERR_EXISTS)
    {
        if (fdt_setprop(fdt, chosen_offset, "bootargs", cmdline, cmdline_size) < 0)
        {
            fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + DTB_PAD_SIZE);
            fdt_setprop(fdt, chosen_offset, "bootargs", cmdline, cmdline_size);
        }
    }

end:
    return fdt_totalsize(fdt);
}

rt_size_t fdt_set_linux_initrd(rt_uint64_t initrd_addr, rt_size_t initrd_size)
{
    rt_uint64_t addr, size_ptr;
    rt_off_t chosen_offset;
    int i;

    if (fdt == RT_NULL)
    {
        goto end;
    }

    chosen_offset = fdt_find_and_add_subnode("chosen");

    /* update the entry */
    for (i = fdt_num_mem_rsv(fdt) - 1; i >= 0; --i)
    {
        fdt_get_mem_rsv(fdt, i, &addr, &size_ptr);
        if (addr == initrd_addr) {
            fdt_del_mem_rsv(fdt, i);
            break;
        }
    }

    /* add the memory */
    if (fdt_add_mem_rsv(fdt, initrd_addr, initrd_size) < 0)
    {
        /* move the memory */
        fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + DTB_PAD_SIZE);
        if (fdt_add_mem_rsv(fdt, initrd_addr, initrd_size) < 0)
        {
            goto end;
        }
    }

    /* install initrd */
    if (chosen_offset >= 0 || chosen_offset == -FDT_ERR_EXISTS)
    {
        chosen_offset = fdt_path_offset(fdt, "/chosen");

        if (IN_64BITS_MODE)
        {
            fdt_setprop_u64(fdt, chosen_offset, "linux,initrd-start", initrd_addr);
            fdt_setprop_u64(fdt, chosen_offset, "linux,initrd-end", initrd_addr + initrd_size);
        }
        else
        {
            fdt_setprop_u32(fdt, chosen_offset, "linux,initrd-start", initrd_addr);
            fdt_setprop_u32(fdt, chosen_offset, "linux,initrd-end", initrd_addr + initrd_size);
        }
    }

end:
    return fdt_totalsize(fdt);
}
