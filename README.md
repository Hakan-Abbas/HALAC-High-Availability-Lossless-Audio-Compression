# HALAC High Availability Lossless Audio Compression
I took a break from HALIC for GDCC 2023. Then I tried something a little different. In the past(2018-2019), I had been working on the lossless audio compression. However, I could not bring together the work I did. Now I have a little time and I think I developed a fast codec. I worked on 16 bit, 2 channel audio data (.wav). Higher bit and channel options can be added if necessary. As a result, the approach is the same.
HALAC, like the HALIC, focuses on a reasonable compression ratio and high processing speed. The compression rate for audio data is usually limited. So I wanted a solution that can work faster with a few percent concessions.
I used a quick estimation with ANS(FSE). I don't know if there are other codecs using ANS, but the majority uses "Rice Coding". GPU or SIMD was not used. Also now in the multihread version.
I tried to find the middle way by working with different music genres.

![sibel_can](https://github.com/Hakan-Abbas/HALAC-High-Availability-Lossless-Audio-Compression-/assets/158841237/acfeeacd-7815-4a25-b1ac-e465c682ebb4)
![HALAC_0 2 4_MT_BENCHMARK1](https://github.com/Hakan-Abbas/HALAC-High-Availability-Lossless-Audio-Compression-/assets/158841237/23c74e3b-1f90-45ec-9d6e-c43593b2c527)
