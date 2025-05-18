# Just a Sample v1.2

This page acts as a tutorial for Just a Sample, and also documents some hidden features and use-cases.

JAS is shorthand for Just a Sample.

## Plugin Interface

![Plugin UI 2.1](Assets/Features//Plugin%20UI%202.1.png)

### Playback Controls
![Playback Controls](Assets/Features/Playback%20Controls.png)

After loading a sound into Just a Sample, the header is the first place to find key playback controls. 

Drag vertically over controls to change their values smoothly. You can also double-click on text to edit with precision.

Most controls can be automated smoothly.

1. Tune the pitch of your sound with the **semitone control** (-12 to +12) and finetune with the **cent control** (-100 to +100).

2. **EXPERIMENTAL**: This control opens a dialog to select a region of your sample. JAS will analyze the pitch of the selected region and automatically set the semitone and cent controls, such that the center A plays at 440hz. This works best on simple sound sources.

<p align="center"><img src="Assets/Features/Detect Pitch.png" width="300"></p>

3. The **attack envelope** controls the volume of your instrument when a note is first activated. You have fine control over the attack length and shape. You should use a short attack time for quicker sounds and a longer attack time for more drawn out sounds. The default value is 1ms, but most sounds will use >20ms for a more natural feel.

4. The **release envelope** controls the volume of your instrument when a note is released. The envelope will trigger automatically when your note is within 5 seconds of the end of the sample.

5. In Basic playback, **Lo-Fi** disables anti-aliasing, resulting in a grittier, retro sound.

6. JAS supports two modes of playback. 

    - In **Basic playback**, sounds are resampled to different pitches by changing their speed. This maintains the quality of the sound but makes it difficult to deal with notes far from center.
    - In **Bungee playback**, sounds are resampled with a complex algorithm to preserve time. This takes very high CPU, but expands the range of functional notes and gives more control over timing. Distortion becomes more noticeable 2-3 octaves from center. Note that resampling to a higher pitch takes more CPU.

7. In Bungee playback, change **playback speed** freely (0.01x to 5x) without affecting pitch. 

8. **Looping** wraps the end of a sound back to its start, allowing you to hold out a note indefinitely. When looping is enabled, JAS allows for separate control of a **sample start** and **sample end** portion. Playback occurs as follows.

    - Begin playback at sample start.
    - At loop end, wrap back around to loop start.
    - When the note is released, jump to the start of the sample end portion, and fully continue playback until sample end.

    JAS uses a power-preserving crossfade to handle these transitions. The crossfade length can be controlled via plugin parameter *Crossfade Samples*.

9. **Mono** mixes the plugin output to mono, averaging the channels. This is reflected visually in the waveform views. 

10. The **gain control** controls the plugin's output volume. This is reflected visually in the waveform views

### Editor and Navigator
![Editor and Navigator](Assets/Features/Editor%20and%20Navigator.png)

11. 

12. 

13. 

14. 

15. 

16. 

17. 

18. 

19. 


#### *Waveform Mode*
<p align="center"><img src="Assets/Features/Waveform Mode.png" width="300"></p>

### Effects 

![Effects](Assets/Features/Effects.png)

20. 
21. 
22. 
23. 
24. 
25. 

### Footer

![Footer](Assets/Features/Footer.png)

26. 
27. 
28. 
29. 
30. 
31. 

<p align="center"><img src="Assets/Features/Dark Mode.png" width="300"></p>

## Other Features
- Pitch Wheel
- Voice count
- Midi start and end
- Reaper integration (dummy param)
- Octave speed factor

Hidden parameters

## Tips and Tricks