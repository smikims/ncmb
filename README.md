ncmb
====

A Mandelbrot set viewer utilizing the ncurses library, with scrolling and zooming.

#Commands

##Normal mode commands
**h,j,k,l, arrow keys** : scroll by a single row or column  
**H,J,K,L** : scroll by a few rows or columns at a time  
**+/=** : zoom in  
**-/\_** : zoom out  
**o** : do a centered zoom out (throws away last zoom stack location)  
**z** : enter zoom box select mode  
**m** : open variable editing menu  
**d** : reset all variables to defaults  
**q** : quit  

##Zoom box mode commands
**h,j,k,l, arrow keys** : move the cursor by a single row or column  
**H,J,K,L** : move the cursor by a few rows or columns at a time  
**return** : begin/end selecting the area to zoom to  
**z** : exit zoom box mode  
**q** : quit  

##Menu mode commands
**j,k, arrow keys** : move up and down between entries  
**m** : exit menu  
**return** : edit value (press enter when done)  
**d** : reset all variables to defaults  
**q** : quit  
