#ifndef __GPU_SRAM_HEAP_H__
#define __GPU_SRAM_HEAP_H__

void ion_gpu_sram_heap_destroy(struct ion_heap *heap);
struct ion_heap *ion_gpu_sram_heap_create(
					struct ion_platform_heap *heap_data);

#endif

