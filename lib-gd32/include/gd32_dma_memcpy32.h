/*
 * gd32_dma_memcpy32.h
 */

#ifndef GD32_DMA_MEMCPY32_H_
#define GD32_DMA_MEMCPY32_H_

#include <cstdint>
#include <cassert>

#include "gd32.h"

namespace dma {
#if !defined (GD32F4XX)
# define DMA_MEMCPY32_DMAx			DMA0
# define DMA_MEMCPY32_DMA_CHx		DMA_CH3
# define DMA_MEMCPY32_DMA_CHMADDR	DMA_CHMADDR
#else
# define DMA_MEMCPY32_DMAx			DMA1
# define DMA_MEMCPY32_DMA_CHx		DMA_CH0
# define DMA_MEMCPY32_DMA_CHMADDR	DMA_CHM0ADDR
#endif

void memcpy32_init() ;

inline void memcpy32(const void *pDestination, const void *pSource, const uint32_t nLength) {
	assert((reinterpret_cast<uint32_t>(pSource) & 0x3) == 0);
	assert((reinterpret_cast<uint32_t>(pDestination) & 0x3) == 0);

	uint32_t dmaCHCTL = DMA_CHCTL(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx);
	dmaCHCTL &= ~DMA_CHXCTL_CHEN;
	DMA_CHCTL(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx) = dmaCHCTL;

    DMA_CHPADDR(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx) = reinterpret_cast<uint32_t>(pSource);
    DMA_MEMCPY32_DMA_CHMADDR(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx) = reinterpret_cast<uint32_t>(pDestination);
    DMA_CHCNT(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx) = (nLength & DMA_CHXCNT_CNT);

    dmaCHCTL |= DMA_CHXCTL_CHEN;
    DMA_CHCTL(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx) = dmaCHCTL;
}

inline bool memcpy32_is_active() {
	return DMA_CHCNT(DMA_MEMCPY32_DMAx, DMA_MEMCPY32_DMA_CHx) != 0;
}
}  // namespace dma

#endif /* GD32_DMA_MEMCPY32_H_ */
