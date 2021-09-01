#include <rtthread.h>
#include <fdt.h>

int fdt_dump(int argc, char** argv)
{
    if (argc > 1 && fdt_load_from_fs(argv[1]) == FDT_LOAD_OK)
    {
        struct dtb_node *dtb_node_list = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));
        if (dtb_node_list != RT_NULL && fdt_get_dtb_list(dtb_node_list) == RT_EOK)
        {
            fdt_get_dts_dump(dtb_node_list);
            /* dtb_node_list will free on here */
            fdt_free_dtb_list(dtb_node_list);
        }
    }
    else
    {
        rt_kprintf("Usage: fdt <dtb_filename>\n");
    }

    return 0;
}
MSH_CMD_EXPORT(fdt_dump, fdt dump from fs);
