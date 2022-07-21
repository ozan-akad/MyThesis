r1 = DefineNumber[0.05];
Point(1) = {0, 0, 0, r1};
Point(2) = {1, 0, 0, r1};
Point(3) = {1, 1, 0,  r1};
Point(4) = {0, 1, 0, r1};
Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};
Line Loop(6) = {4, 1, 2, 3};
Plane Surface(6) = {6};
Physical Line("Inflow",2) = {3};
Physical Line("Wall",1)   = {1,2,4};

Physical Surface("Domain") = {6};

# element sayısı 230 icin r = 0.12
# element sayısı 1054 icin r = 0.05 
# element sayısı 1562 icin r = 0.0425
# element sayısı 2484 icin r = 0.03315
