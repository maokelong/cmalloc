# cmalloc

## 简介
一款基于混合异构内存的数据元数据解耦的并发内存分配器。

## 版本发布
- 2017-04-14 提交了 v1.0 版本。
    - 单线程原型机；仅支持单线程负载，仅实现了 malloc 及 free 。
- 2017-04-21 提交了 v2.0 版本。
    - 多线程原型机；在单线程原型机的基础上添加了起码的多线程支持。
- 2017-04-24 提交了 v2.1 版本。
    - 简单地实现了远程释放(remote free)；使得线程之间交叉释放内存的行为也是线程安全的了；
    - 每个 CPU 核心持有独立的私有堆池；这样可以减少 cache miss；
    - 在 SMB 内添加了一个计数器，用于统计已分配内存数量；释放内存后某 SMB 已分配内存为 0，则将其状态转换为 cold；分配内存后某 SMB 已分配内存量达到 100%，则将其状态转换为 warm；
    - 简单地清理了一下代码。

## 使用方法
1. 静态链接
2. 动态链接
3. LD_PRELOAD

## 测试说明
进入 testcases 目录。

首先通过 `make` 将 cmalloc 硬编码到测试用例中，然后通过 `make pintest` 对测试用例进行初步分析。

测试用例的二进制程序及分析结果均保存在 testcases 目录下。

> **注意**：makefile 暂时仅支持单 PinTool，可能会在未来添加可拓展性。

## API
### 标准 API
- `void *aligned_alloc(size_t alignment, size_t size);`

- `void *calloc(size_t nmemb, size_t size);`

- `void free(void *ptr);`

- `void *malloc(size_t size);`

- `void *realloc(void *ptr, size_t size);`

### 额外 API
- `int mallopt(int parameter_number, int parameter_value);`

> **注意**：目前仅实现了 malloc 及 free 的接口，且未经有效地测试。

## 关于

感谢 ptmalloc、hoard、streamflow、ssmalloc、supermalloc，及 scalloc 等用户态内存分配器的作者，你们在各期刊会议上发表的论文，让我得以了解现代内存分配器的设计思想；你们在 github 上公布的源码，让我得以瞥见现代内存分配器的实现细节。

本内存分配器中的很多设计都是有根的，有些本人考证过出处，而有些则没有。考证过出处的包括：源自 hoard 的线程本地加速缓冲、源自 streamflow 的远程释放、源自 CAMA 的 cache-set awear 的思想、源自 supermalloc 的超大数组，以及源自 scalloc 的虚拟跨度等；未考证过的包括：风险指针、ABA 计数器，以及无锁队列等。此外本内存分配器中有部分代码摘自 stackoverflow 及一些采用 BSD license 的内存分配器，如 scalloc，ssmalloc等。

本内存分配器所支持的标准 API 均为 C11 中介绍的标准内存管理接口，非标准 API 中的 mallopt 启发自 glibc malloc。
