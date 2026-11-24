#pragma once

// Original 2D Win64 trial has no C runtime, sprintf, iostream or locale.
// Present genuine race timing to hundredths rather than truncating a
// 65.99-second result to an indistinguishable "65".
namespace latebrake_preview
{
inline void PreviewTimeUnavailable(char (&output)[16]) noexcept
{
    output[0]='-';output[1]='-';output[2]='.';
    output[3]='-';output[4]='-';output[5]=0;
}
inline bool FormatPreviewCentiseconds(
    unsigned int centiseconds,char (&output)[16]) noexcept
{
    if(centiseconds>99999999u)
    {
        PreviewTimeUnavailable(output);return false;
    }
    unsigned int seconds=centiseconds/100u;
    char reverseDigits[12];
    int count=0;
    do{
        reverseDigits[count++]=static_cast<char>(
            '0'+seconds%10u);
        seconds/=10u;
    }while(seconds>0u && count<11);
    int n=0;
    while(count>0)output[n++]=reverseDigits[--count];
    output[n++]='.';
    output[n++]=static_cast<char>(
        '0'+(centiseconds/10u)%10u);
    output[n++]=static_cast<char>(
        '0'+centiseconds%10u);
    output[n]=0;
    return true;
}
inline bool FormatPreviewRaceSeconds(
    float seconds,bool available,char (&output)[16]) noexcept
{
    // Comparisons reject NaN/infinity. Cap BEFORE casting floats to ints.
    if(!available || !(seconds>=0.0f && seconds<999999.0f))
    {
        PreviewTimeUnavailable(output);return false;
    }
    return FormatPreviewCentiseconds(
        static_cast<unsigned int>(seconds*100.0f+0.5f),output);
}
inline bool FormatPreviewRecordMilliseconds(
    int milliseconds,char (&output)[16]) noexcept
{
    if(milliseconds<=0 || milliseconds>999999990)
    {
        PreviewTimeUnavailable(output);return false;
    }
    // Integer arithmetic preserves source record accuracy across platforms.
    return FormatPreviewCentiseconds(
        static_cast<unsigned int>((milliseconds+5)/10),output);
}
} // namespace latebrake_preview
