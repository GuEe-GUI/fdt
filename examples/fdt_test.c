#include <rtthread.h>
#include <fdt.h>

int fdt_test()
{
    if (fdt_load_from_fs("vexpress-v2p-ca9.dtb") == FDT_LOAD_OK)
    {
        struct dtb_node *dtb_node_list = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));
        if (dtb_node_list != RT_NULL && fdt_get_dtb_list(dtb_node_list) == RT_EOK)
        {
            struct dtb_node *serial0 = fdt_get_dtb_node_by_path(dtb_node_list, "/smb@4000000/motherboard/iofpga@7,00000000/uart@9000");
            struct dtb_node *cpu = fdt_get_dtb_node_by_path(dtb_node_list, "/cpus");
            struct dtb_node *user1 = fdt_get_dtb_node_by_path(dtb_node_list, "/smb@4000000/motherboard/leds/user1");

            if (serial0 != RT_NULL)
            {
                int property_size;
                rt_uint32_t u32_value;
                rt_uint32_t *u32_ptr;
                char *str_ptr;

                rt_kprintf("name = %s\n", serial0->name);

                rt_kputs("reg = <");
                for_each_property_cell(serial0, "reg", u32_value, u32_ptr, property_size)
                {
                    rt_kprintf("0x%x,", u32_value);
                }
                rt_kputs("\b>;\n");

                rt_kputs("compatible = ");
                for_each_property_string(serial0, "compatible", str_ptr, property_size)
                {
                    rt_kprintf("\"%s\",", str_ptr);
                }
                rt_kputs("\b;\n");
            }

            if (cpu != RT_NULL)
            {
                struct dtb_node *cpu_child = cpu;

                rt_kprintf("\nname = %s\n", cpu->name);

                for_each_node_child(cpu_child)
                {
                    rt_kprintf("path = %s\n", cpu_child->path);
                }
            }

            if (user1 != RT_NULL)
            {
                struct dtb_node *user = user1;

                rt_kprintf("\nname = %s, lable = %s\n", user1->name, fdt_get_dtb_node_property(user1, "label", RT_NULL));

                for_each_node_sibling(user)
                {
                    rt_kprintf("name = %s, lable = %s\n", user->name, fdt_get_dtb_node_property(user, "label", RT_NULL));
                }
            }

            fdt_free_dtb_list(dtb_node_list);
        }
    }

    if (fdt_load_from_fs("bcm2711-rpi-4-b.dtb") == FDT_LOAD_OK)
    {
        struct dtb_node *dtb_node_list = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));
        if (dtb_node_list != RT_NULL && fdt_get_dtb_list(dtb_node_list) == RT_EOK)
        {
            struct dtb_node *bt_pins;
            int memreserve_size;
            struct dtb_memreserve *memreserve;

            bt_pins = fdt_get_dtb_node_by_name_DFS(dtb_node_list, "bt_pins");
            bt_pins = fdt_get_dtb_node_by_name_BFS(dtb_node_list, bt_pins->name);
            bt_pins = fdt_get_dtb_node_by_phandle_DFS(dtb_node_list, bt_pins->handle);
            bt_pins = fdt_get_dtb_node_by_phandle_BFS(dtb_node_list, bt_pins->handle);

            memreserve = fdt_get_dtb_memreserve(bt_pins, &memreserve_size);
            if (memreserve_size > 0)
            {
                int i;
                for (i = 0; i < memreserve_size; ++i)
                {
                    rt_kputs("\n/memreserve/\t");
                    if (IN_64BITS_MODE)
                    {
                        rt_kprintf("0x%016x 0x%016x;", memreserve[i].address, memreserve[i].size);
                    }
                    else
                    {
                        rt_kprintf("0x%08x%08x 0x%08x%08x;", memreserve[i].address, memreserve[i].size);
                    }
                }
            }

            if (bt_pins != RT_NULL)
            {
                int property_size;
                rt_uint8_t u8_value;
                rt_uint8_t *u8_ptr;

                rt_kprintf("\n\nphandle = <0x%x>\n", bt_pins->handle);
                rt_kprintf("name = %s\n", bt_pins->name);
                rt_kprintf("path = %s\n", bt_pins->path);

                rt_kputs("brcm,pins = [");
                for_each_property_byte(bt_pins, "brcm,pins", u8_value, u8_ptr, property_size)
                {
                    rt_kprintf("%02x ", u8_value);
                }
                rt_kputs("\b]\n");
            }

            fdt_free_dtb_list(dtb_node_list);
        }
    }
    return 0;
}
MSH_CMD_EXPORT(fdt_test, fdt API test);