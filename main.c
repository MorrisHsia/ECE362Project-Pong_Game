#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdint.h>
#include <stdio.h>


// These are function pointers.  They can be called like functions
// after you set them to point to other functions.
// e.g.  cmd = bitbang_cmd;
// They will be set by the stepX() subroutines to point to the new
// subroutines you write below.
void (*cmd)(char b) = 0;
void (*data)(char b) = 0;
void (*display1)(const char *) = 0;
void (*display2)(const char *) = 0;

// Prototypes for subroutines in support.c
void generic_lcd_startup(void);
void clock(void);
void step1(void);
void step2(void);
void step3(void);
void step4(void);
void step6(void);

// This array will be used with dma_display1() and dma_display2() to mix
// commands that set the cursor location at zero and 64 with characters.
//
uint16_t dispmem[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};

//=========================================================================
// Subroutines for step 2.
//=========================================================================
void spi_cmd(char b) {
    // Your code goes here.
	while((SPI2->SR & SPI_SR_TXE) == 0);
	SPI2->DR = b;
}

void spi_data(char b) {
    // Your code goes here.
	while((SPI2->SR & SPI_SR_TXE) == 0);
	SPI2->DR = 0x200 + b;
}

void spi_init_lcd(void) {
    // Your code goes here.
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->MODER &= ~(0xcf000000);
	GPIOB->MODER |= 0x8a000000;
	GPIOB->AFR[1] &= ~0xf0ff000;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	SPI2->CR1 &= ~SPI_CR1_SPE;
	SPI2->CR1 |= SPI_CR1_MSTR;
	SPI2->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2;
	SPI2->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
	//SPI2->CR1 &= ~SPI_CR1_CPOL & ~SPI_CR1_CPHA;
	SPI2->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP | SPI_CR2_DS_3 | SPI_CR2_DS_0;
	SPI2->CR2 &= ~SPI_CR2_DS_1 & ~SPI_CR2_DS_2;
	SPI2->CR1 |= SPI_CR1_SPE;
	generic_lcd_startup();
}

//===========================================================================
// Subroutines for step 3.
//===========================================================================

// Display a string on line 1 using DMA.
void dma_display1(const char *s) {
    // Your code goes here.
    cmd(0x80 + 0);
    int x;
    for(x=0; x<16; x+=1){
        if (s[x])
    		dispmem[x+1] = s[x] + 0x200;
        else
            break;
    }
    for(   ; x<16; x+=1){
		dispmem[x+1] = s[x] + 0x220;
    }
    dma_spi_init_lcd();
}

void dma_spi_init_lcd(void) {
    // Your code goes here.
	//step3
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	DMA1_Channel5->CCR &= ~DMA_CCR_EN;
	DMA1_Channel5->CMAR = (uint32_t) dispmem;
	DMA1_Channel5->CPAR = (uint32_t) &(SPI2->DR);
	DMA1_Channel5->CNDTR = 17;
	DMA1_Channel5->CCR |= DMA_CCR_DIR;
	DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
	DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
	DMA1_Channel5->CCR |= DMA_CCR_MINC;
	DMA1_Channel5->CCR &= ~DMA_CCR_PL;
	SPI2->CR1 &= ~SPI_CR1_SPE;
	SPI2->CR2 |= SPI_CR2_TXDMAEN;
	SPI2->CR1 |= SPI_CR1_SPE;
	DMA1_Channel5->CCR |= DMA_CCR_EN;
	//step4
	/*spi_init_lcd();
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	DMA1_Channel5->CCR &= ~DMA_CCR_EN;
	DMA1_Channel5->CMAR = (uint32_t) dispmem;
	DMA1_Channel5->CPAR = (uint32_t) &(SPI2->DR);
	DMA1_Channel5->CNDTR = 34;
	DMA1_Channel5->CCR |= DMA_CCR_DIR;
	DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
	DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
	DMA1_Channel5->CCR |= DMA_CCR_MINC;
	DMA1_Channel5->CCR |= DMA_CCR_CIRC;
	DMA1_Channel5->CCR &= ~DMA_CCR_PL;
	SPI2->CR1 &= ~SPI_CR1_SPE;
	SPI2->CR2 |= SPI_CR2_TXDMAEN;
	SPI2->CR1 |= SPI_CR1_SPE;
	DMA1_Channel5->CCR |= DMA_CCR_EN;*/
}

//===========================================================================
// Subroutines for Step 4.
//===========================================================================

// Display a string on line 1 by copying a string into the
// memory region circularly moved into the display by DMA.
void circdma_display1(const char *s) {
    // Your code goes here.
    int x;
    for(x=0; x<16; x+=1){
        if (s[x]){
    		dispmem[x+1] =s[x] +  0x200;
        }
        else{
            break;
        }
    }
    for(   ; x<16; x+=1){
		dispmem[x+1] = 0x220;
    }
}

//===========================================================================
// Display a string on line 2 by copying a string into the
// memory region circularly moved into the display by DMA.
void circdma_display2(const char *s) {
    int x;
    for(x=0; x<16; x+=1){
        if (s[x]){
    		dispmem[x+18] =s[x] +  0x200;
        }
        else{
            break;
        }
    }
    for(   ; x<16; x+=1){
		dispmem[x+18] =  0x220;
    }
}

//===========================================================================
// Subroutines for Step 6.
//===========================================================================


void init_tim2(void) {
    // Your code goes here.
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_DIR;
	TIM2->PSC = 48000-1;
	TIM2->ARR = 100-1;
	TIM2->DIER |= TIM_DIER_UIE;//update
	TIM2->CR1 |= TIM_CR1_CEN;
	NVIC->ISER[0] = 1<<TIM2_IRQn;
}
void TIM2_IRQHandler() {
	TIM2->SR &= ~TIM_SR_UIF;//clear the UIF to prevent endless IRQ
	clock();
}

int main(void)
{
    step1();
    //step2();
    //step3();
    //step4();
    //step6();
}
