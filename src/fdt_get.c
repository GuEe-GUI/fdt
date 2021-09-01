#include "libfdt/libfdt.h"
#include "fdt.h"

#define DTB_ALL_NODES_PATH_SIZE (32 * 1024)

/* we maybe use to booting linux or change the dtb data so set it as a global variable */
void *fdt = RT_NULL;
static rt_err_t pub_status;

static struct
{
    const char *ptr;
    const char *end;
    char *cur;
} paths_buf = {RT_NULL, RT_NULL};

static rt_err_t _fdt_get_dtb_properties_list(struct dtb_property *dtb_property, rt_off_t node_off)
{
    /* caller alrealy checked fdt */
    rt_off_t property_off = fdt_first_property_offset(fdt, node_off);
    struct fdt_property *fdt_property;

    if (property_off < 0)
    {
        return FDT_GET_EMPTY;
    }

    for (;;)
    {
        fdt_property = (struct fdt_property *)fdt_get_property_by_offset(fdt, property_off, &dtb_property->size);
        if (fdt_property != RT_NULL)
        {
            dtb_property->name = fdt_string(fdt, fdt32_to_cpu(fdt_property->nameoff));
            dtb_property->value = fdt_property->data;
            dtb_property->size = fdt32_to_cpu(fdt_property->len);
        }

        property_off = fdt_next_property_offset(fdt, property_off);
        if (property_off >= 0)
        {
            dtb_property->next = (struct dtb_property *)rt_malloc(sizeof(struct dtb_property));
            if (dtb_property->next == RT_NULL)
            {
                return FDT_NO_MEMORY;
            }
            dtb_property = dtb_property->next;
        }
        else
        {
            dtb_property->next = RT_NULL;
            break;
        }
    }

    return FDT_GET_OK;
}

static rt_err_t _fdt_get_dtb_nodes_list(struct dtb_node *dtb_node_head, struct dtb_node *dtb_node, const char *pathname)
{
    rt_off_t root_off;
    rt_off_t node_off;
    int pathname_sz;
    int node_name_sz;

    /* caller alrealy checked fdt */
    if ((root_off = fdt_path_offset(fdt, pathname)) >= 0)
    {
        pathname_sz = rt_strlen(pathname);
        node_off = fdt_first_subnode(fdt, root_off);

        if (node_off < 0)
        {
            return FDT_GET_EMPTY;
        }

        for (;;)
        {
            dtb_node->parent = dtb_node_head;
            dtb_node->sibling = RT_NULL;
            dtb_node->name = fdt_get_name(fdt, node_off, &node_name_sz);

            /* parent_path + name + '/' + '\0' */
            if (paths_buf.cur + pathname_sz + node_name_sz + 2 < paths_buf.end)
            {
                dtb_node->path = (const char *)paths_buf.cur;
                rt_strncpy(paths_buf.cur, pathname, pathname_sz);
                paths_buf.cur += pathname_sz;
                rt_strncpy(paths_buf.cur, (char *)dtb_node->name, node_name_sz);
                paths_buf.cur += node_name_sz;
                *paths_buf.cur++ = '/';
                *paths_buf.cur++ = '\0';
            }
            else
            {
                dtb_node->path = RT_NULL;
                rt_kprintf("\033[31m\rERROR: `DTB_ALL_NODES_PATH_SIZE' = %d bytes is configured too low.\033[0m\n", DTB_ALL_NODES_PATH_SIZE);
                return FDT_NO_MEMORY;
            }

            dtb_node->handle = fdt_get_phandle(fdt, node_off);
            dtb_node->properties = (struct dtb_property *)rt_malloc(sizeof(struct dtb_property));
            dtb_node->child = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));

            if (dtb_node->properties == RT_NULL || dtb_node->child == RT_NULL)
            {
                return FDT_NO_MEMORY;
            }

            pub_status = _fdt_get_dtb_properties_list(dtb_node->properties, node_off);
            if (pub_status == FDT_GET_EMPTY)
            {
                rt_free(dtb_node->properties);
                dtb_node->properties = RT_NULL;
            }
            else if (pub_status != FDT_GET_OK)
            {
                return pub_status;
            }

            pub_status = _fdt_get_dtb_nodes_list(dtb_node, dtb_node->child, dtb_node->path);
            if (pub_status == FDT_GET_EMPTY)
            {
                rt_free(dtb_node->child);
                dtb_node->child = RT_NULL;
            }
            else if (pub_status != FDT_GET_OK)
            {
                return pub_status;
            }

            node_off = fdt_next_subnode(fdt, node_off);
            if (node_off >= 0)
            {
                dtb_node->sibling = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));
                if (dtb_node->sibling == RT_NULL)
                {
                    return FDT_NO_MEMORY;
                }
                dtb_node = dtb_node->sibling;
            }
            else
            {
                break;
            }
        }
    }

    return FDT_GET_OK;
}

rt_err_t fdt_get_dtb_list(struct dtb_node *dtb_node_head)
{
    int root_off;

    if (fdt == RT_NULL)
    {
        return FDT_NO_LOADED;
    }
    if (dtb_node_head == RT_NULL)
    {
        return FDT_GET_EMPTY;
    }
    if (paths_buf.ptr == RT_NULL)
    {
        paths_buf.ptr = rt_malloc(DTB_ALL_NODES_PATH_SIZE);
        if (paths_buf.ptr == RT_NULL)
        {
            return FDT_NO_MEMORY;
        }
        paths_buf.cur = (char *)paths_buf.ptr;
        paths_buf.end = (char *)(paths_buf.ptr + DTB_ALL_NODES_PATH_SIZE);
    }

    root_off = fdt_path_offset(fdt, "/");

    dtb_node_head->header = rt_malloc(sizeof(struct dtb_header));
    if (dtb_node_head->header == RT_NULL)
    {
        return FDT_NO_MEMORY;
    }
    else
    {
        ((struct dtb_header *)dtb_node_head->header)->root = '/';
        ((struct dtb_header *)dtb_node_head->header)->zero = '\0';
        ((struct dtb_header *)dtb_node_head->header)->memreserve_sz = fdt_num_mem_rsv(fdt);

        if (dtb_node_head->header->memreserve_sz > 0)
        {
            int i;
            int memreserve_sz = dtb_node_head->header->memreserve_sz;
            rt_uint32_t off_mem_rsvmap = fdt_off_mem_rsvmap(fdt);
            struct fdt_reserve_entry *rsvmap = (struct fdt_reserve_entry *)((char *)fdt + off_mem_rsvmap);

            ((struct dtb_header *)dtb_node_head->header)->memreserve = rt_malloc(sizeof(struct dtb_memreserve) * memreserve_sz);
            if (dtb_node_head->header->memreserve == RT_NULL)
            {
                return FDT_NO_MEMORY;
            }
            for (i = 0; i < memreserve_sz; ++i)
            {
                ((struct dtb_header *)dtb_node_head->header)->memreserve[i].address = fdt64_to_cpu(rsvmap[i].address);
                ((struct dtb_header *)dtb_node_head->header)->memreserve[i].size = fdt64_to_cpu(rsvmap[i].size);
            }
        }
        else
        {
            ((struct dtb_header *)dtb_node_head->header)->memreserve = RT_NULL;
        }
    }

    dtb_node_head->path = paths_buf.ptr;
    *paths_buf.cur++ = '/';
    *paths_buf.cur++ = '\0';
    dtb_node_head->parent = RT_NULL;
    dtb_node_head->sibling = RT_NULL;

    dtb_node_head->handle = fdt_get_phandle(fdt, root_off);
    dtb_node_head->properties = (struct dtb_property *)rt_malloc(sizeof(struct dtb_property));
    dtb_node_head->child = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));

    if (dtb_node_head->properties == RT_NULL || dtb_node_head->child == RT_NULL)
    {
        return FDT_NO_MEMORY;
    }

    pub_status = _fdt_get_dtb_properties_list(dtb_node_head->properties, root_off);
    if (pub_status != FDT_GET_OK)
    {
        return pub_status;
    }

    pub_status = _fdt_get_dtb_nodes_list(dtb_node_head, dtb_node_head->child, dtb_node_head->path);

    /* paths_buf.ptr addr save in the dtb_node_head's path */
    paths_buf.ptr = RT_NULL;
    paths_buf.cur = RT_NULL;

    return pub_status;
}

static void _fdt_free_dtb_node(struct dtb_node *dtb_node)
{
    struct dtb_node *dtb_node_last;
    struct dtb_property *dtb_property;
    struct dtb_property *dtb_property_last;

    while (dtb_node != RT_NULL)
    {
        dtb_property = dtb_node->properties;
        while (dtb_property != RT_NULL)
        {
            dtb_property_last = dtb_property;
            dtb_property = dtb_property->next;
            rt_free(dtb_property_last);
        }

        _fdt_free_dtb_node(dtb_node->child);

        dtb_node_last = dtb_node;
        dtb_node = dtb_node->sibling;
        rt_free(dtb_node_last);
    }
}

void fdt_free_dtb_list(struct dtb_node *dtb_node_head)
{
    if (dtb_node_head == RT_NULL)
    {
        return;
    }

    /* only root node can free all path buffer */
    if (dtb_node_head->parent == RT_NULL || !rt_strcmp(dtb_node_head->path, "/"))
    {
        rt_free((void *)dtb_node_head->path);
        rt_free((void *)dtb_node_head->header->memreserve);
        rt_free((void *)dtb_node_head->header);
    }

    _fdt_free_dtb_node(dtb_node_head);
}

static void _fdt_printf_depth(int depth)
{
    int i = depth;
    while (i --> 0)
    {
        rt_kputs("\t");
    }
}

static rt_bool_t _fdt_test_string_list(const void *value, int size)
{
    const char *str = value;
    const char *str_start, *str_end;

    if (size == 0)
    {
        return RT_FALSE;
    }

    /* string end with '\0' */
    if (str[size - 1] != '\0')
    {
        return RT_FALSE;
    }

    /* get string list end */
    str_end = str + size;

    while (str < str_end)
    {
        str_start = str;
        /* before string list end, not '\0' and a printable characters */
        while (str < str_end && *str && ((unsigned char)*str >= ' ' && (unsigned char)*str <= '~'))
        {
            ++str;
        }

        /* not zero, or not increased */
        if (*str != '\0' || str == str_start)
        {
            return RT_FALSE;
        }

        /* next string */
        ++str;
    }

    return RT_TRUE;
}

static void _fdt_printf_dtb_node_info(struct dtb_node *dtb_node)
{
    static int depth = 0;
    struct dtb_property *dtb_property;

    while (dtb_node != RT_NULL)
    {
        rt_kputs("\n");
        _fdt_printf_depth(depth);
        rt_kputs(dtb_node->name);
        rt_kputs(" {\n");
        ++depth;

        dtb_property = dtb_node->properties;
        while (dtb_property != RT_NULL)
        {
            _fdt_printf_depth(depth);

            rt_kputs(dtb_property->name);

            if (dtb_property->size > 0)
            {
                int size = dtb_property->size;
                char *value = dtb_property->value;

                rt_kputs(" = ");
                if (_fdt_test_string_list(value, size) == RT_TRUE)
                {
                    /* print string list */
                    char *str = value;
                    do
                    {
                        rt_kprintf("\"%s\"", str);
                        str += rt_strlen(str) + 1;
                        rt_kputs(", ");
                    } while (str < value + size);
                    rt_kputs("\b\b");
                }
                else if ((size % 4) == 0)
                {
                    /* print addr and size cell */
                    int i;
                    fdt32_t *cell = (fdt32_t *)value;

                    rt_kputs("<");
                    for (i = 0, size /= 4; i < size; ++i)
                    {
                        rt_kprintf("0x%x ", fdt32_to_cpu(cell[i]));
                    }
                    rt_kputs("\b>");
                }
                else
                {
                    /* print bytes array */
                    int i;
                    rt_uint8_t *byte = (rt_uint8_t *)value;

                    rt_kputs("[");
                    for (i = 0; i < size; ++i)
                    {
                       rt_kprintf("%02x ", *byte++);
                    }
                    rt_kputs("\b]");
                }
            }
            rt_kputs(";\n");
            dtb_property = dtb_property->next;
        }

        _fdt_printf_dtb_node_info(dtb_node->child);
        dtb_node = dtb_node->sibling;

        --depth;
        _fdt_printf_depth(depth);
        rt_kputs("};\n");
    }
}

void fdt_get_dts_dump(struct dtb_node *dtb_node_head)
{
    if (dtb_node_head != RT_NULL)
    {
        int i = dtb_node_head->header->memreserve_sz;

        rt_kputs("/dts-v1/;\n");

        while (i --> 0)
        {
            if (IN_64BITS_MODE)
            {
                rt_kprintf("\n/memreserve/\t0x%016x 0x%016x;", dtb_node_head->header->memreserve[i].address, dtb_node_head->header->memreserve[i].size);
            }
            else
            {
                rt_kprintf("\n/memreserve/\t0x%08x%08x 0x%08x%08x;", dtb_node_head->header->memreserve[i].address, dtb_node_head->header->memreserve[i].size);
            }
        }

        _fdt_printf_dtb_node_info(dtb_node_head);
    }
}

struct dtb_node *fdt_get_dtb_node_by_name_DFS(struct dtb_node *dtb_node, const char *nodename)
{
    struct dtb_node *dtb_node_child;

    while (dtb_node != RT_NULL)
    {
        if (!rt_strcmp(nodename, dtb_node->name))
        {
            return dtb_node;
        }

        dtb_node_child = fdt_get_dtb_node_by_name_DFS(dtb_node->child, nodename);

        if (dtb_node_child != RT_NULL)
        {
            return dtb_node_child;
        }
        dtb_node = dtb_node->sibling;
    }

    return RT_NULL;
}

struct dtb_node *fdt_get_dtb_node_by_name_BFS(struct dtb_node *dtb_node, const char *nodename)
{
    if (dtb_node != RT_NULL)
    {
        struct dtb_node *dtb_node_child, *dtb_node_head = dtb_node;

        while (dtb_node != RT_NULL)
        {
            if (!rt_strcmp(nodename, dtb_node->name))
            {
                return dtb_node;
            }
            dtb_node = dtb_node->sibling;
        }

        dtb_node = dtb_node_head;

        while (dtb_node != RT_NULL)
        {
            dtb_node_child = fdt_get_dtb_node_by_name_BFS(dtb_node->child, nodename);

            if (dtb_node_child != RT_NULL)
            {
                return dtb_node_child;
            }
            dtb_node = dtb_node->sibling;
        }
    }

    return RT_NULL;
}

struct dtb_node *fdt_get_dtb_node_by_path(struct dtb_node *dtb_node, const char *pathname)
{
    int i = 0;
    char *node_name;
    char *pathname_clone;
    int pathname_sz;

    if (pathname == RT_NULL || dtb_node == RT_NULL)
    {
        return RT_NULL;
    }

    /* skip '/' */
    if (*pathname == '/')
    {
        ++pathname;
    }

    /* root not have sibling, so skip */
    if (dtb_node->parent == RT_NULL || !rt_strcmp(dtb_node->path, "/"))
    {
        dtb_node = dtb_node->child;
    }

    pathname_sz = rt_strlen(pathname) + 1;
    pathname_clone = rt_malloc(pathname_sz);
    if (pathname_clone == RT_NULL)
    {
        return RT_NULL;
    }
    rt_strncpy(pathname_clone, pathname, pathname_sz);
    node_name = pathname_clone;

    while (pathname_clone[i])
    {
        if (pathname_clone[i] == '/')
        {
            /* set an end of name that can used to strcmp */
            pathname_clone[i] = '\0';

            while (dtb_node != RT_NULL)
            {
                if (!rt_strcmp(dtb_node->name, node_name))
                {
                    break;
                }
                dtb_node = dtb_node->sibling;
            }
            if (dtb_node == RT_NULL)
            {
                rt_free(pathname_clone);
                return RT_NULL;
            }
            dtb_node = dtb_node->child;
            node_name = &pathname_clone[i + 1];
        }
        ++i;
    }

    /*
     *  found the end node and it's name:
     *      (pathname[pathname_sz - 1]) is '\0'
     *      (&pathname_clone[i] - node_name) is the node_name's length
     */
    node_name = &((char *)pathname)[(pathname_sz - 1) - (&pathname_clone[i] - node_name)];
    rt_free(pathname_clone);
    while (dtb_node != RT_NULL)
    {
        if (!rt_strcmp(dtb_node->name, node_name))
        {
            return dtb_node;
        }
        dtb_node = dtb_node->sibling;
    }

    return RT_NULL;
}

struct dtb_node *fdt_get_dtb_node_by_phandle_DFS(struct dtb_node *dtb_node, phandle handle)
{
    struct dtb_node *dtb_node_child;

    while (dtb_node != RT_NULL)
    {
        if (dtb_node->handle == handle)
        {
            return dtb_node;
        }

        dtb_node_child = fdt_get_dtb_node_by_phandle_DFS(dtb_node->child, handle);

        if (dtb_node_child != RT_NULL)
        {
            return dtb_node_child;
        }
        dtb_node = dtb_node->sibling;
    }

    return RT_NULL;
}

struct dtb_node *fdt_get_dtb_node_by_phandle_BFS(struct dtb_node *dtb_node, phandle handle)
{
    if (dtb_node != RT_NULL)
    {
        struct dtb_node *dtb_node_child, *dtb_node_head = dtb_node;

        while (dtb_node != RT_NULL)
        {
            if (dtb_node->handle == handle)
            {
                return dtb_node;
            }
            dtb_node = dtb_node->sibling;
        }

        dtb_node = dtb_node_head;

        while (dtb_node != RT_NULL)
        {
            dtb_node_child = fdt_get_dtb_node_by_phandle_BFS(dtb_node->child, handle);

            if (dtb_node_child != RT_NULL)
            {
                return dtb_node_child;
            }
            dtb_node = dtb_node->sibling;
        }
    }

    return RT_NULL;
}

void fdt_get_dtb_node_cells(struct dtb_node *dtb_node, int *addr_cells, int *size_cells)
{
    if (dtb_node != RT_NULL && addr_cells != RT_NULL && size_cells != RT_NULL)
    {
        struct dtb_property *dtb_property;
        *addr_cells = -1;
        *size_cells = -1;

        /* if couldn't found, check parent */
        while ((dtb_node = dtb_node->parent) != RT_NULL)
        {
            dtb_property = dtb_node->properties;
            while (dtb_property != RT_NULL)
            {
                if (!rt_strcmp(dtb_property->name, "#address-cells"))
                {
                    *addr_cells = fdt32_to_cpu(*(int *)dtb_property->value);
                }
                else if (!rt_strcmp(dtb_property->name, "#size-cells"))
                {
                    *size_cells = fdt32_to_cpu(*(int *)dtb_property->value);
                }
                if (*addr_cells != -1 && *size_cells != -1)
                {
                    return;
                }
                dtb_property = dtb_property->next;
            }
        }

        if (*addr_cells == -1)
        {
            *addr_cells = FDT_ROOT_ADDR_CELLS_DEFAULT;
        }
        if (*size_cells == -1)
        {
            *size_cells = FDT_ROOT_SIZE_CELLS_DEFAULT;
        }
    }
}

void *fdt_get_dtb_node_property(struct dtb_node *dtb_node, const char *property_name, int *property_size)
{
    if (dtb_node != RT_NULL && property_name != RT_NULL)
    {
        struct dtb_property *dtb_property = dtb_node->properties;

        while (dtb_property != RT_NULL)
        {
            if (!rt_strcmp(dtb_property->name, property_name))
            {
                if (property_size != RT_NULL)
                {
                    *property_size = dtb_property->size;
                }
                return dtb_property->value;
            }
            dtb_property = dtb_property->next;
        }
    }

    return RT_NULL;
}

struct dtb_memreserve *fdt_get_dtb_memreserve(struct dtb_node *dtb_node, int *memreserve_size)
{
    if (dtb_node != RT_NULL && memreserve_size != RT_NULL)
    {
        struct dtb_node *dtb_node_root = dtb_node;

        while (dtb_node_root != RT_NULL)
        {
            if (!rt_strcmp(dtb_node_root->path, "/"))
            {
                break;
            }
            dtb_node_root = dtb_node_root->parent;
        }

        *memreserve_size = dtb_node_root->header->memreserve_sz;

        return dtb_node_root->header->memreserve;
    }
    return RT_NULL;
}

rt_bool_t fdt_get_dtb_node_status(struct dtb_node *dtb_node)
{
    if (dtb_node != RT_NULL)
    {
        char *status = fdt_get_dtb_node_property(dtb_node, "status", RT_NULL);
        if (status != RT_NULL)
        {
            return (!rt_strcmp(status, "okay") || !rt_strcmp(status, "ok")) ? RT_TRUE : RT_FALSE;
        }

        return RT_TRUE;
    }

    return RT_FALSE;
}

char *fdt_get_dtb_string_list_value(void *value, int size, int index)
{
    int i = 0;
    char *str = value;

    if (str != RT_NULL)
    {
        do
        {
            if (i++ == index)
            {
                return str;
            }
            str += rt_strlen(str) + 1;
        } while (str < (char *)value + size);
    }

    return RT_NULL;
}

char *fdt_get_dtb_string_list_value_next(void *value, void *end)
{
    char *str = value;

    if (str != RT_NULL)
    {
        str += rt_strlen(str) + 1;
        if (str < (char *)end)
        {
            return str;
        }
    }

    return RT_NULL;
}

rt_uint32_t fdt_get_dtb_cell_value(void *value)
{
    fdt32_t *cell = (fdt32_t *)value;

    return fdt32_to_cpu(*cell);
}

rt_uint8_t fdt_get_dtb_byte_value(void *value)
{
    rt_uint8_t *byte = (rt_uint8_t *)value;

    return *byte;
}
