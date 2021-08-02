float fp_accumulate(float window0[32])
{
    float result = 0;
    for (int x=0;x<32;x++){
        result += window0[x];
    }
    return result;
}