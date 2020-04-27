/* ========================================
 *
 *Programa para medir señal análogica tranformandola
 *a digital y luego enviarla a la PC para se procesada 
 *Para esto se utiliza un PSOC 5lp 
 *
 * ========================================
*/
#include "project.h"

#define BUFER    64u

    /* Defines for DMA */
#define DMA_BYTES_PER_BURST 2
#define DMA_REQUEST_PER_BURST 1
#define DMA_SRC_BASE (CYDEV_PERIPH_BASE)
#define DMA_DST_BASE (CYDEV_SRAM_BASE)

/* Variable declarations for DMA */
/* Move these variable declarations to the top of the function */
uint8 DMA_Chan;
uint8 DMA_TD[1];
uint8 DMA_done = 0;

/*Variable de salida de endpoint desde el USBUART_cdc.c*/
//extern volatile uint8 USBUART_cdc_data_in_ep;
/* variable donde se almacenan las muestra del adc*/
//uint8 adc_muestra[BUFER] = {0};
uint16 bufferIn[BUFER]={0}, length;
uint8 bufferOut[8];
uint16 adcReading = 0u;

int main(void)
{
    /*****************DMA*********************************/


/* DMA Configuration for DMA */
DMA_Chan = DMA_DmaInitialize(DMA_BYTES_PER_BURST, DMA_REQUEST_PER_BURST, 
    HI16(DMA_SRC_BASE), HI16(DMA_DST_BASE));
DMA_TD[0] = CyDmaTdAllocate();
CyDmaTdSetConfiguration(DMA_TD[0], BUFER, DMA_TD[0], DMA__TD_TERMOUT_EN | CY_DMA_TD_INC_DST_ADR);
CyDmaTdSetAddress(DMA_TD[0], LO16((uint32)ADC_DEC_SAMP_PTR), LO16((uint32)bufferIn[128]));
CyDmaChSetInitialTd(DMA_Chan, DMA_TD[0]);
CyDmaChEnable(DMA_Chan, 1);

CyGlobalIntEnable; // Habilitar interrupciones 
ADC_Start();
USB_Start(0, USB_3V_OPERATION); // Iniciar el USB periferico
while(!USB_GetConfiguration()); // Esperar hasta que el USB configure 
USB_EnableOutEP(2); // Habilitar el Endpoint de salida (EP2)
  
   
/*Registre los búferes SRAM para los puntos finales EP IN y OUT. */
USB_LoadInEP (1, (uint8 *)bufferIn, 64);
USB_ReadOutEP(2, bufferOut, length);
//USBUART_LoadInEP(USBUART_cdc_data_in_ep, adc_muestra, BUFER);
    

/* Iniciamos los elementos principales del programa */
ADC_Start();        // inicio del ADC
ADC_IRQ_Disable();  // La interrupción del adc se desabilita  
isr_Done_Start();   // Iniciamos la interrupción isr_Done
ADC_StartConvert(); // Enpezamos la conversión de la señal 
    

    for(;;)
    {
        /* Place your application code here. */
        if(DMA_done)
        {
            int i = 0;
            for(i =0;i<64;++i)
            {
                ADC_StartConvert(); // Iniciamos la conversión de datos del ADC
                
                ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);// Esperamos los resultados del ADC
                
                adcReading= ADC_GetResult16(); // Datos de ADD
                bufferIn[i] = adcReading; 
            }    
            if(USB_GetEPState(2) == USB_OUT_BUFFER_EMPTY)
            {//if we have data
               length = USB_GetEPCount(2); // Obtenga la longitud de los datos recibidos
               USB_ReadOutEP(2, bufferOut, length); // Obtener Datos 
            }
            while(USB_GetEPState(1) != USB_IN_BUFFER_EMPTY)
            {
               // Espere hasta que nuestro Endpoint de entrada  esté vacío
            }
            USB_LoadInEP (1, (uint8 *)bufferIn, 64);
            USB_ReadOutEP(2, bufferOut, length);
            USB_LoadInEP(1, NULL   , 64); // Echo the data back into the buffer  
            
            
            
            
            /* Hacemos un alto en el ADC para trasferir los datos en el bufér*/
            ADC_StopConvert();
            
         
        }
    }
}

/* [] Finl del programa */
