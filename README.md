# fda0 4coder 4.1.7 customization
This layer is build to depend on https://github.com/ryanfleury/4coder_fleury
But code in ```4fda0_definitions_lister.cpp``` should work on vanilla 4coder.




```4coder_fleury_fda0.cpp``` is included in ```4coder_fleury.cpp``` around here:
```
...
#include "4coder_fleury_casey.cpp"
#include "4coder_fleury_hooks.cpp"

//~ NOTE(fda0): My includes
#include "4coder_fleury_fda0.cpp"

//~ NOTE(rjf): Plots Demo File
#include "4coder_fleury_plots_demo.cpp"
...
```



And I added this to ```buildsuper_x64-win.bat``` for easier building:
```
...
set opts=%opts% -I"../4coder_fda0"
...
```