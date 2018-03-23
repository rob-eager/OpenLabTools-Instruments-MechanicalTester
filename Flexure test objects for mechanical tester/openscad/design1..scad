$fn=30;
legth=40;
width=10;
height=10;
difference(){
    union(){
        cube([legth,width,height]);//horizontal cube
        translate([35,width/2,0])cube([legth-30,width,height/2],center=true);
        cube([legth-30,width,height*12]);//vertical cube
        translate([5,15,117.5])cube([legth-30,height,height/2],center=true);
    }
    translate([35,width/2,-30])cylinder(r=1.5*1.1,h=height*10,center=true);
    //translate([35,width/2,8.5])cylinder(r=2.5*1.1,h=1.5,center=true);
    rotate([90,0,0])translate([27,1.5,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([27,8.5,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,1.5,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,8.5,0])cylinder(r=3,h=height*10,center=true);
    translate([5,width+4,100])cylinder(r=1.5*1.1,h=height*10,center=true);
    rotate([90,0,0])translate([1,112,0,])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([9,112,0,])cylinder(r=3,h=height*10,center=true);
}
   
       