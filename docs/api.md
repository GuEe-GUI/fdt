# fdt load API

## 从文件系统上读取设备树
```c
rt_err_t fdt_load_from_fs(char *dtb_filename)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_filename       | 设备树文件名                         |
| **返回**          | **描述**                            |
|FDT_LOAD_OK        | 正确执行                            |
|FDT_LOAD_EMPTY     | 加载文件失败                         |
|FDT_NO_MEMORY      | 内存不足                            |
|FDT_LOAD_ERROR     | 设备树格式不正确                     |

## 从内存上读取设备树
```c
rt_err_t fdt_load_from_memory(void *dtb_ptr)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_ptr            | 设备树在内存上的内存地址              |
| **返回**          | **描述**                            |
|FDT_LOAD_OK        | 正确执行                            |
|FDT_LOAD_ERROR     | 设备树格式不正确                     |
|FDT_LOAD_EMPTY     | 内存地址为空                         |

为了支持多种场景开发（例如bootloader），fdt加载后会一直驻留在内存上。

# fdt set API

## 设置Linux启动参数
```c
rt_size_t fdt_set_linux_cmdline(char *cmdline)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|cmdline            | 启动参数                            |
| **返回**          | **描述**                            |
|rt_size_t          | 修改设备树后设备树的大小              |

## 设置Linux init ramdisk
```c
rt_size_t fdt_set_linux_initrd(rt_uint64_t initrd_addr, rt_size_t initrd_size)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|initrd_addr        | init ramdisk 内存地址               |
|initrd_size        | init 大小                           |
| **返回**          | **描述**                            |
|rt_size_t          | 修改设备树后设备树的大小              |

# fdt get API

## 将原始设备树转换为设备节点树
```c
rt_err_t fdt_get_dtb_list(struct dtb_node *dtb_node_head)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node_head      | 设备节点树头节点，需要手动申请空间     |
| **返回**          | **描述**                            |
|FDT_GET_OK         | 转换成功                            |
|FDT_NO_LOADED      | 设备树还未加载进内存                 |
|FDT_NO_MEMORY      | 内存不足                            |

## 释放设备节点树内存
```c
void fdt_free_dtb_list(struct dtb_node *dtb_node_head)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node_head      | 设备节点树头节点                     |
| **返回**          | **描述**                            |
|无返回值            | 无描述                              |

示例

```c
#include <rtthread.h>
#include <fdt.h>

int main()
{
    if (fdt_load_from_fs("sample.dtb") == FDT_LOAD_OK)
    {
        struct dtb_node *dtb_node_list = (struct dtb_node *)rt_malloc(sizeof(struct dtb_node));

        if (dtb_node_list != RT_NULL && fdt_get_dtb_list(dtb_node_list) == RT_EOK)
        {
            /* load dtb node list OK */
        }
        /* success or fail, dtb_node_list will free on here */
        fdt_free_dtb_list(dtb_node_list);
    }

    return 0;
}
```

## 打印设备树信息
```c
void fdt_get_dts_dump(struct dtb_node *dtb_node_head)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node_head      | 设备节点树头节点                     |
| **返回**          | **描述**                            |
|无返回值           | 无描述                              |

## 通过节点名称查找节点
```c
struct dtb_node *fdt_get_dtb_node_by_name_DFS(struct dtb_node *dtb_node, const char *nodename)
```
```c
struct dtb_node *fdt_get_dtb_node_by_name_BFS(struct dtb_node *dtb_node, const char *nodename)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
|nodename           | 查找节点的名称                       |
| **返回**          | **描述**                            |
|struct dtb_node *  | 返回查找的节点，                     |
|RT_NULL            | 未找到为RT_NULL                     |

## 通过节点路径查找节点
```c
struct dtb_node *fdt_get_dtb_node_by_path(struct dtb_node *dtb_node, const char *pathname)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
|pathname           | 查找节点的路径                       |
| **返回**          | **描述**                            |
|struct dtb_node *  | 返回查找的节点，                     |
|RT_NULL            | 未找到为RT_NULL                     |

## 通过节点phandle值查找节点
```c
struct dtb_node *fdt_get_dtb_node_by_phandle_DFS(struct dtb_node *dtb_node, phandle handle)
```
```c
struct dtb_node *fdt_get_dtb_node_by_phandle_BFS(struct dtb_node *dtb_node, phandle handle)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
|handle             | 查找节点的phandle值                  |
| **返回**          | **描述**                            |
|struct dtb_node *  | 返回查找的节点，                     |
|RT_NULL            | 未找到为RT_NULL                     |

DFS和BFS是用于区分不同深度节点的搜索方法，时间复杂度和空间复杂度都较高，建议使用路径查找。

## 读取节点属性值的单位
```c
void fdt_get_dtb_node_cells(struct dtb_node *dtb_node, int *addr_cells, int *size_cells)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
|addr_cells         | 返回的地址块的单位（u32）大小         |
|size_cells         | 返回的尺寸块的单位（u32）大小         |
| **返回**          | **描述**                            |
|无返回值           | 无描述                               |


## 读取节点属性值
```c
void *fdt_get_dtb_node_property(struct dtb_node *dtb_node, const char *property_name, int *property_size)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
|property_name      | 属性名称                            |
|property_size      | 属性大小                            |
| **返回**          | **描述**                            |
|void *           | 无描述                                |
|RT_NULL            | 该设备树没有该属性                   |

## 读取预留内存信息
```c
struct dtb_memreserve *fdt_get_dtb_memreserve(struct dtb_node *dtb_node, int *memreserve_size)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
|memreserve_size    | 返回的内存信息数组长度                |
| **返回**          | **描述**                            |
|struct dtb_memreserve *| 内存预留信息数组的内存地址        |
|RT_NULL            | 该设备树没有预留内存信息              |

## 读取节点的状态
```c
rt_bool_t fdt_get_dtb_node_status(struct dtb_node *dtb_node)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|dtb_node           | 设备节点树节点                       |
| **返回**          | **描述**                            |
|RT_FALSE           | 状态为禁用                          |
|RT_TRUE            | 状态为使用                          |

## 读取属性值中的字符串
```c
char *fdt_get_dtb_string_list_value(void *value, int size, int index)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|value              | 节点属性的内存地址                   |
|size               | 节点属性的尺寸                       |
|index              | 要读取字符串的索引                   |
| **返回**          | **描述**                            |
|char *             | 字符串的内存地址                     |
|RT_NULL            | 该索引没有字符串                     |

## 读取值为字符串属性下一个字符串
```c
char *fdt_get_dtb_string_list_value_next(void *value, void *end)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|value              | 节点属性的内存地址                   |
|end                | 节点属性的结束地址                   |
| **返回**          | **描述**                            |
|char *             | 字符串的内存地址                     |
|RT_NULL            | 没有下一个字符串                     |

## 读取值为u32属性的值
```c
rt_uint32_t fdt_get_dtb_cell_value(void *value)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|value              | 节点属性的内存地址                   |
| **返回**          | **描述**                            |
|rt_uint32_t        | 该值小端的值                         |

## 读取值为u8属性的值
```c
rt_uint8_t fdt_get_dtb_byte_value(void *value)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|value              | 节点属性的内存地址                   |
| **返回**          | **描述**                            |
|rt_uint8_t         | 该值小端的值                        |

# fdt foreach API

## 遍历属性中字符串宏
```c
for_each_property_string(node_ptr, property_name, str, size)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|node_ptr           | 设备节点树节点                       |
|property_name      | 节点属性名称                         |
|str                | 每次遍历到的字符串                   |
|size               | 用于保存节点属性的尺寸                |

## 遍历属性中u32宏
```c
for_each_property_cell(node_ptr, property_name, value, list, size)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|node_ptr           | 设备节点树节点                       |
|property_name      | 节点属性名称                         |
|value              | 每次遍历到的值                       |
|list               | 用于保存节点属性的值内存地址          |
|size               | 用于保存节点属性的尺寸                |

## 遍历属性中u8宏
```c
for_each_property_byte(node_ptr, property_name, value, list, size)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|node_ptr           | 设备节点树节点                       |
|property_name      | 节点属性名称                         |
|value              | 每次遍历到的值                       |
|list               | 用于保存节点属性的值内存地址          |
|size               | 用于保存节点属性的尺寸                |

## 遍历子节点宏
```c
for_each_node_child(node_ptr)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|node_ptr           | 设备节点树节点                       |

## 遍历兄弟节点宏
```c
for_each_node_sibling(node_ptr)
```

| 参数              | 描述                                |
|:------------------|:------------------------------------|
|node_ptr           | 设备节点树节点                       |
