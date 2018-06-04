DEFINE_GRADIENT_PALETTE(flame_palette_fire){
    0, 0, 0, 0,          
    168, 255, 0, 0,      
    224, 255, 140, 0,    
    255, 255, 240, 204}; 
DEFINE_GRADIENT_PALETTE(flame_palette_blue){
    0, 0, 0, 0, 
    90, 9, 9, 91,
    172, 10, 29, 157,
    210, 3, 59, 180,
    255, 166, 194, 252};
DEFINE_GRADIENT_PALETTE(flame_palette_hotwarm){
    0, 0, 0, 0, 
    40, 25, 0, 154,
    80, 235, 46, 166,
    127, 231, 5, 71,
    170, 246, 150, 12,
    211, 205, 85, 46,
    255, 255, 255, 255}; 
DEFINE_GRADIENT_PALETTE(flame_palette_green){
    0, 0, 0, 0, 
    127, 23, 94, 0,
    220, 62, 210, 10,  
    255, 22, 253, 98}; 
DEFINE_GRADIENT_PALETTE(flame_palette_violette){
    0, 0, 0, 0, 
    85, 101, 3, 91,
    164, 190, 8, 250,   
    255, 250, 70, 250};

const TProgmemRGBGradientPalettePtr gFlamePalettes[] = {
    flame_palette_fire,
    flame_palette_blue,
    flame_palette_hotwarm,
    flame_palette_green,
    flame_palette_violette};

const uint8_t gFlamePalettesCount = sizeof(gFlamePalettes) / sizeof(TProgmemRGBGradientPalettePtr);
