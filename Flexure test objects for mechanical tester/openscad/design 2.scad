$fn=30;
legth=40;
width=10;
height=10;
difference(){
    union(){
        cube([legth,width,height]);
        translate([35,width/2,10])cube([legth-30,width,height/2],center=true);
    }
    translate([5,width/2,-30])cylinder(r=1.5*1.1,h=height*10,center=true);
    translate([35,width/2,-30])cylinder(r=1.5*1.1,h=height*10,center=true);
    rotate([90,0,0])translate([27,0,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([27,10,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,0,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,10,0])cylinder(r=3,h=height*10,center=true);
}
translate([45,0,0])
   difference(){
    union(){
        cube([legth,width,height]);
        translate([35,width/2,10])cube([legth-30,width,height/2],center=true);
    }
    translate([5,width/2,-30])cylinder(r=1.5*1.1,h=height*10,center=true);
    translate([35,width/2,-30])cylinder(r=1.5*1.1,h=height*10,center=true);
    rotate([90,0,0])translate([27,0,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([27,10,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,0,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,10,0])cylinder(r=3,h=height*10,center=true);
}