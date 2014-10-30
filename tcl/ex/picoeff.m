function [a, b, sampling] = picoeff(N,R,C)
% This function gives the coefficients "a" and "b" of the PI 
% controller at sampling frequency  "sampling". N is the minimum 
% number of flows, R the maximum round trip delay and C the capacity 
% of the link in (average sized) packets/sec.
%function [a, b, sampling] = picoeff(N,R,C)
N = N*500;
C = C*500;
p_q = 1/R;
w_g = 2*N/R/R/C;
K = w_g*abs( (i*w_g/p_q+1)/((R*C)^3/(2*N)^2));
num = K*[1/w_g 1];
den = [1 0];
[numn,denn] = bilinear(num,den,100*w_g);
sampling = 100*w_g;
format long 
a = numn(1);
b = -numn(2);
