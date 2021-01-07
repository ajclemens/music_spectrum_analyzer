#include <arduinoFFT.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define SAMPLES 64            //Must be a power of 2
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW   // Set display type  so that  MD_MAX72xx library treets it properly
#define MAX_DEVICES  4   // Total number display modules
#define CLK_PIN   13  // Clock pin to communicate with display
#define DATA_PIN  11  // Data pin to communicate with display
#define CS_PIN    10  // Control pin to communicate with display
#define  xres 32      // Total number of  columns in the display, must be <= SAMPLES/2
#define  yres 8       // Total number of  rows in the display


int MY_ARRAY[]={0, 128, 192, 224, 240, 248, 252, 254, 255}; // default = standard pattern
int MY_MODE_1[]={0, 128, 192, 224, 240, 248, 252, 254, 255}; // standard pattern

 
double vReal[SAMPLES];
double vImag[SAMPLES];
char data_avgs[xres];

int yvalue;
int displaycolumn , displayvalue;
int peaks[xres];
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);   // display object
arduinoFFT FFT = arduinoFFT();                                    // FFT object
 


void setup() {
    
    ADCSRA = 0b11100101;      // set ADC to free running mode and set pre-scalar to 32 (0xe5)
    ADMUX = 0b00000000;       // use pin A0 and external voltage reference
    mx.begin();           // initialize display
    delay(50);            // wait to get reference voltage stabilized
}
 
void loop() {
   // ++ Sampling
   for(int i=0; i<SAMPLES; i++)
    {
      while(!(ADCSRA & 0x10));        // wait for ADC to complete current conversion ie ADIF bit set
      ADCSRA = 0b11110101 ;               // clear ADIF bit so that ADC can do next operation (0xf5)
      int value = ADC - 512 ;                 // Read from ADC and subtract DC offset caused value
      vReal[i]= value/8;                      // Copy to bins after compressing
      vImag[i] = 0;                         
    }
    // -- Sampling

 
    // ++ FFT
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    // -- FFT

    
    // ++ re-arrange FFT result to match with no. of columns on display ( xres )
    int step = (SAMPLES/2)/xres; 
    int c=0;
    for(int i=0; i<(SAMPLES/2); i+=step)  
    {
      data_avgs[c] = 0;
      for (int k=0 ; k< step ; k++) {
          data_avgs[c] = data_avgs[c] + vReal[i+k];
      }
      data_avgs[c] = data_avgs[c]/step; 
      c++;
    }
    // -- re-arrange FFT result to match with no. of columns on display ( xres )

    
    // ++ send to display according measured value 
    for(int i=0; i<xres; i++)
    {
      data_avgs[i] = constrain(data_avgs[i],0,80);            // set max & min values for buckets
      data_avgs[i] = map(data_avgs[i], 0, 80, 0, yres);        // remap averaged values to yres
      yvalue=data_avgs[i];

      peaks[i] = peaks[i]-1;    // decay by one light
      if (yvalue > peaks[i]) 
          peaks[i] = yvalue ;
      yvalue = peaks[i];    
      displayvalue=MY_ARRAY[yvalue];
      displaycolumn=31-i;
      mx.setColumn(displaycolumn, displayvalue);              // for left to right
     }
     // -- send to display according measured value 
     
} 
