namespace SCREEN {
    extern u16 *LCD;
    extern u32 dirty;
    void LCDWrite( u32 cd, u16 v );
    void LCDReset();
}
