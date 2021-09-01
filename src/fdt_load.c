#include <rthw.h>
#include <dfs_posix.h>

#include "libfdt/libfdt.h"
#include "fdt.h"

extern void *fdt;

rt_inline rt_err_t fdt_check()
{
    return fdt_check_header(fdt) == 0 ? FDT_LOAD_OK : FDT_LOAD_ERROR;
}

rt_err_t fdt_load_from_fs(char *dtb_filename)
{
    rt_base_t level = rt_hw_interrupt_disable();

    int fd = -1;
    rt_size_t dtb_sz;
    rt_err_t status = FDT_LOAD_OK;

    if (dtb_filename == RT_NULL)
    {
        status = FDT_LOAD_EMPTY;
        goto end;
    }

    fd = open(dtb_filename, O_RDONLY, 0);

    if (fd == -1)
    {
        status = FDT_LOAD_EMPTY;
        goto end;
    }

    dtb_sz = lseek(fd, 0, SEEK_END);
    if (dtb_sz > 0)
    {
        /* if was loaded before */
        if (fdt != RT_NULL)
        {
            rt_free(fdt);
        }

        fdt = (struct fdt_header *)rt_malloc(sizeof(rt_uint8_t) * dtb_sz);

        if (fdt == RT_NULL)
        {
            status = FDT_NO_MEMORY;
            goto end;
        }

        lseek(fd, 0, SEEK_SET);
        read(fd, fdt, sizeof(rt_uint8_t) * dtb_sz);

        if ((status = fdt_check()) != FDT_LOAD_OK)
        {
            rt_free(fdt);
        }
    }
    else
    {
        status = FDT_LOAD_ERROR;
        goto end;
    }

end:
    if (fd != -1)
    {
        close(fd);
    }
    rt_hw_interrupt_enable(level);

    return status;
}

rt_err_t fdt_load_from_memory(void *dtb_ptr)
{
    if (dtb_ptr == RT_NULL)
    {
        return FDT_LOAD_EMPTY;
    }

    fdt = dtb_ptr;

    if (fdt_check() != FDT_LOAD_OK)
    {
        fdt = RT_NULL;
        return FDT_LOAD_ERROR;
    }
    else
    {
        return FDT_LOAD_OK;
    }
}
