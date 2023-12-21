// Generated case design for EPD/EPD.kicad_pcb
// By https://github.com/revk/PCBCase
// Generated 2023-12-21 17:23:09
// title:	Waveshare EPD adapter
// rev:	1
// company:	Adrian Kennard, Andrews & Arnold Ltd
//

// Globals
margin=0.500000;
lip=2.000000;
casebase=1.000000;
casetop=6.000000;
casewall=3.000000;
fit=0.000000;
edge=1.000000;
pcbthickness=1.200000;
nohull=false;
hullcap=1.000000;
hulledge=1.000000;
useredge=true;

module outline(h=pcbthickness,r=0){linear_extrude(height=h)offset(r=r)polygon(points=[[1.000000,0.000000],[27.000001,0.000001],[27.382684,0.076121],[27.707107,0.292894],[27.923880,0.617318],[28.000001,1.000001],[28.000001,36.000001],[27.923881,36.382684],[27.707108,36.707107],[27.382684,36.923880],[27.000001,37.000001],[1.000002,37.000000],[0.617319,36.923880],[0.292896,36.707107],[0.076123,36.382683],[0.000002,36.000000],[0.000000,27.500001],[0.076121,27.117318],[0.292894,26.792896],[0.617317,26.576123],[1.000000,26.500001],[4.200001,26.500001],[4.458820,26.465926],[4.700001,26.366026],[4.907108,26.207107],[5.066027,26.000001],[5.165928,25.758820],[5.200001,25.500001],[5.200002,10.500000],[5.165926,10.241181],[5.066025,10.000001],[4.907107,9.792894],[4.700001,9.633975],[4.458821,9.534074],[4.200002,9.500000],[1.000001,9.500001],[0.617318,9.423881],[0.292895,9.207108],[0.076122,8.882684],[0.000001,8.500001],[0.000000,1.000000],[0.076121,0.617317],[0.292894,0.292895],[0.617317,0.076122]],paths=[[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43]]);}

module pcb(h=pcbthickness,r=0){linear_extrude(height=h)offset(r=r)polygon(points=[[1.000000,0.000000],[27.000001,0.000001],[27.382684,0.076121],[27.707107,0.292894],[27.923880,0.617318],[28.000001,1.000001],[28.000001,36.000001],[27.923881,36.382684],[27.707108,36.707107],[27.382684,36.923880],[27.000001,37.000001],[1.000002,37.000000],[0.617319,36.923880],[0.292896,36.707107],[0.076123,36.382683],[0.000002,36.000000],[0.000000,27.500001],[0.076121,27.117318],[0.292894,26.792896],[0.617317,26.576123],[1.000000,26.500001],[4.200001,26.500001],[4.458820,26.465926],[4.700001,26.366026],[4.907108,26.207107],[5.066027,26.000001],[5.165928,25.758820],[5.200001,25.500001],[5.200002,10.500000],[5.165926,10.241181],[5.066025,10.000001],[4.907107,9.792894],[4.700001,9.633975],[4.458821,9.534074],[4.200002,9.500000],[1.000001,9.500001],[0.617318,9.423881],[0.292895,9.207108],[0.076122,8.882684],[0.000001,8.500001],[0.000000,1.000000],[0.076121,0.617317],[0.292894,0.292895],[0.617317,0.076122]],paths=[[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43]]);}
spacing=44.000001;
pcbwidth=28.000001;
pcblength=37.000001;
// Parts to go on PCB (top)
module parts_top(part=false,hole=false,block=false){
translate([20.000001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([19.150001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([13.500001,9.600001,1.200000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([24.400001,13.000001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([17.450001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([15.750001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([2.900001,30.600001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([13.000001,18.000001,1.200000])rotate([0,0,90.000000])m2(part,hole,block); // U4 (back)
translate([23.000001,11.000001,1.200000])rotate([0,0,-10.000000])m3(part,hole,block); // D6 (back)
translate([6.700001,9.600001,1.200000])rotate([0,0,180.000000])m4(part,hole,block); // RevK:C_0603 C_0603_1608Metric (back)
translate([25.700001,33.800001,1.200000])rotate([0,0,90.000000])m5(part,hole,block); // Q1 (back)
translate([16.600001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([0.700001,29.700001,1.200000])rotate([0,0,-90.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([24.400001,16.900001,1.200000])rotate([0,0,180.000000])m4(part,hole,block); // RevK:C_0603 C_0603_1608Metric (back)
translate([3.450001,7.867501,1.200000])m6(part,hole,block,2); // J4 (back)
translate([10.200001,7.867501,1.200000])m6(part,hole,block,2); // J4 (back)
translate([22.400001,33.500001,1.200000])rotate([0,0,180.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([21.900001,16.200001,1.200000])rotate([0,0,-90.000000])m4(part,hole,block); // RevK:C_0603 C_0603_1608Metric (back)
translate([14.850001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([22.700001,35.300001,1.200000])rotate([0,0,90.000000])m4(part,hole,block); // RevK:C_0603 C_0603_1608Metric (back)
translate([2.900001,31.700001,1.200000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([24.700001,35.700001,1.200000])rotate([0,0,-90.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([26.500001,10.500001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([25.400001,26.800001,1.200000])rotate([0,0,180.000000])m3(part,hole,block); // D6 (back)
translate([26.500001,11.400001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([24.400001,19.500001,1.200000])rotate([0,0,180.000000])rotate([-0.000000,-0.000000,-90.000000])m7(part,hole,block); // RevK:L_4x4_ TYA4020 (back)
translate([2.900001,27.700001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([16.950001,7.867501,1.200000])m6(part,hole,block,2); // J4 (back)
translate([2.900001,28.800001,1.200000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([26.200001,13.000001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([13.100001,8.600001,1.200000])rotate([0,0,180.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([26.800001,35.700001,1.200000])rotate([0,0,90.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([18.300001,35.600001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([26.900001,16.200001,1.200000])rotate([0,0,-90.000000])m4(part,hole,block); // RevK:C_0603 C_0603_1608Metric (back)
translate([14.000001,28.300001,1.200000])rotate([0,0,180.000000])m8(part,hole,block); // J1 (back)
translate([25.400001,30.800001,1.200000])rotate([0,0,180.000000])m3(part,hole,block); // D6 (back)
translate([23.800001,35.700001,1.200000])rotate([0,0,-90.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([22.500001,13.000001,1.200000])rotate([0,0,180.000000])m1(part,hole,block); // RevK:R_0402_ R_0402_1005Metric (back)
translate([21.500001,23.100001,1.200000])rotate([0,0,90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([24.400001,14.900001,1.200000])rotate([0,0,180.000000])m9(part,hole,block); // RevK:SOT-23-6-MD8942 SOT-23-6 (back)
translate([2.000001,35.000001,1.200000])rotate([0,0,90.000000])m10(part,hole,block); // D4 (back)
translate([23.600001,5.200001,1.200000])rotate([0,0,90.000000])translate([0.000000,-1.050000,0.000000])rotate([90.000000,-0.000000,-0.000000])m11(part,hole,block); // RevK:USB-C-Socket-H CSP-USC16-TR (back)
translate([24.400001,23.700001,1.200000])rotate([0,0,180.000000])rotate([-0.000000,-0.000000,-90.000000])m7(part,hole,block); // RevK:L_4x4_ TYA4020 (back)
translate([25.400001,28.800001,1.200000])rotate([0,0,180.000000])m3(part,hole,block); // D6 (back)
translate([6.300001,8.600001,1.200000])rotate([0,0,180.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
translate([0.700001,28.000001,1.200000])rotate([0,0,-90.000000])m0(part,hole,block); // RevK:C_0402 C_0402_1005Metric (back)
}

parts_top=11;
// Parts to go on PCB (bottom)
module parts_bottom(part=false,hole=false,block=false){
}

parts_bottom=0;
module b(cx,cy,z,w,l,h){translate([cx-w/2,cy-l/2,z])cube([w,l,h]);}
module m0(part=false,hole=false,block=false)
{ // RevK:C_0402 C_0402_1005Metric
// 0402 Capacitor
if(part)
{
	b(0,0,0,1.5,0.65,0.2); // Pad size
	b(0,0,0,1.0,0.5,1); // Chip
}
}

module m1(part=false,hole=false,block=false)
{ // RevK:R_0402_ R_0402_1005Metric
// 0402 Resistor
if(part)
{
	b(0,0,0,1.5,0.65,0.2); // Pad size
	b(0,0,0,1.0,0.5,0.5); // Chip
}
}

module m2(part=false,hole=false,block=false)
{ // U4
// ESP32-S3-MINI-1
translate([-15.4/2,-15.45/2,0])
{
	if(part)
	{
		cube([15.4,20.5,0.8]);
		translate([0.7,0.5,0])cube([14,13.55,2.4]);
	}
	if(hole)
	{
		cube([15.4,20.5,0.8]);
	}
}
}

module m3(part=false,hole=false,block=false)
{ // D6
// SOD-123 Diode
if(part)
{
	b(0,0,0,2.85,1.8,1.35); // part
	b(0,0,0,4.2,1.2,0.7); // pads
}
}

module m4(part=false,hole=false,block=false)
{ // RevK:C_0603 C_0603_1608Metric
// 0603 Capacitor
if(part)
{
	b(0,0,0,1.6,0.95,0.2); // Pad size
	b(0,0,0,1.6,0.8,1); // Chip
}
}

module m5(part=false,hole=false,block=false)
{ // Q1
// SOT-23
if(part)
{
	b(0,0,0,1.4,3.0,1.1); // Body
	b(-0.9375,-0.95,0,1.475,0.6,0.5); // Pad
	b(-0.9375,0.95,0,1.475,0.6,0.5); // Pad
	b(0.9375,0,0,1.475,0.6,0.5); // Pad
}
}

module m6(part=false,hole=false,block=false,N=0)
{ // J4
// PTSM socket
if(part)
{
	hull()
	{
		b(0,-7.5/2+0.3,0,1.7+N*2.5,7.5,4);
		b(0,-7.5/2+0.3,0,1.7+N*2.5-2,7.5,5);
	}
	// Pins
	for(p=[0:N-1])translate([-2.5*(N-1)/2+p*2.5,0,-2.1])cylinder(r1=0.3,r2=1,h=2.1);
}
if(hole)
{
	b(0,-10.5/2-7.5+0.3,0,1.1+N*2.5,10.5,5);
}
}

module m7(part=false,hole=false,block=false)
{ // RevK:L_4x4_ TYA4020
// 4x4 Inductor
if(part)
{
	b(0,0,0,4,4,3);
}
}

module m8(part=false,hole=false,block=false)
{ // J1
// FPC 24 pin connector
{
	if(part)
	{
		b(0,0,0,11.8,3.9,0.5); // Pins
		b(0,-0.6,0,15.8,5.2,2.1); // Connector
	}
	if(hole)
	{
		b(0,0,0.5,15,25,0.1); // Ribbon
	}
}
}

module m9(part=false,hole=false,block=false)
{ // RevK:SOT-23-6-MD8942 SOT-23-6
// SOT-23-6
if(part)
{
	b(0,0,0,1.726,3.026,1.2); // Part
	b(0,0,0,3.6,2.5,0.5); // Pins
}
}

module m10(part=false,hole=false,block=false)
{ // D4
// 1x1mm LED
if(part)
{
	b(0,0,0,1,1,.8);
}
if(hole)
{
	b(0,0,.8,1,1,100);
}
}

module m11(part=false,hole=false,block=false)
{ // RevK:USB-C-Socket-H CSP-USC16-TR
// USB connector
rotate([-90,0,0])translate([-4.47,-3.84,0])
{
	if(part)
	{
		translate([1.63,0,1.63])
		rotate([-90,0,0])
		hull()
		{
			cylinder(d=3.26,h=7.75,$fn=24);
			translate([5.68,0,0])
			cylinder(d=3.26,h=7.75,$fn=24);
		}
		translate([0,6.65,0])cube([8.94,1.1,1.63]);
		translate([0,2.2,0])cube([8.94,1.6,1.63]);
	}
	if(hole)
	{
		// Plug
		translate([1.63,-20,1.63])
		rotate([-90,0,0])
		hull()
		{
			cylinder(d=2.5,h=21,$fn=24);
			translate([5.68,0,0])
			cylinder(d=2.5,h=21,$fn=24);
		}
		translate([1.63,-22.5,1.63])
		rotate([-90,0,0])
		hull()
		{
			cylinder(d=7,h=21,$fn=24);
			translate([5.68,0,0])
			cylinder(d=7,h=21,$fn=24);
		}
	}
}

}

// Generate PCB casework

height=casebase+pcbthickness+casetop;
$fn=48;

module pyramid()
{ // A pyramid
 polyhedron(points=[[0,0,0],[-height,-height,height],[-height,height,height],[height,height,height],[height,-height,height]],faces=[[0,1,2],[0,2,3],[0,3,4],[0,4,1],[4,3,2,1]]);
}


module pcb_hulled(h=pcbthickness,r=0)
{ // PCB shape for case
	if(useredge)outline(h,r);
	else hull()outline(h,r);
}

module solid_case(d=0)
{ // The case wall
	hull()
        {
                translate([0,0,-casebase])pcb_hulled(height,casewall-edge);
                translate([0,0,edge-casebase])pcb_hulled(height-edge*2,casewall);
        }
}

module preview()
{
	pcb();
	color("#0f0")parts_top(part=true);
	color("#0f0")parts_bottom(part=true);
	color("#f00")parts_top(hole=true);
	color("#f00")parts_bottom(hole=true);
	color("#00f")parts_top(block=true);
	color("#00f")parts_bottom(block=true);
}

module top_half()
{
	difference()
	{
		translate([-casebase-100,-casewall-100,pcbthickness-lip/2+0.01]) cube([pcbwidth+casewall*2+200,pcblength+casewall*2+200,height]);
		translate([0,0,pcbthickness-lip/2-0.01])pcb_hulled(lip,casewall/2+fit);
	}
}

module bottom_half()
{
	translate([-casebase-100,-casewall-100,pcbthickness+lip/2-height-0.01]) cube([pcbwidth+casewall*2+200,pcblength+casewall*2+200,height]);
	translate([0,0,pcbthickness-lip/2])pcb_hulled(lip,casewall/2-fit);
}

module case_wall()
{
	difference()
	{
		solid_case();
		translate([0,0,-height])pcb_hulled(height*2);
	}
}

module top_side_hole()
{
	intersection()
	{
		parts_top(hole=true);
		case_wall();
	}
}

module bottom_side_hole()
{
	intersection()
	{
		parts_bottom(hole=true);
		case_wall();
	}
}

module parts_space()
{
	minkowski()
	{
		union()
		{
			parts_top(part=true,hole=true);
			parts_bottom(part=true,hole=true);
		}
		sphere(r=margin,$fn=6);
	}
}

module top_cut()
{
	difference()
	{
		top_half();
		if(parts_top)difference()
		{
			minkowski()
			{ // Penetrating side holes
				top_side_hole();
				rotate([180,0,0])
				pyramid();
			}
			minkowski()
			{
				top_side_hole();
				cylinder(d=margin,h=height,$fn=4);
			}
		}
	}
	if(parts_bottom)difference()
	{
		minkowski()
		{ // Penetrating side holes
			bottom_side_hole();
			pyramid();
		}
			minkowski()
			{
				bottom_side_hole();
				translate([0,0,-height])cylinder(d=margin,h=height,$fn=4);
			}
	}
}

module bottom_cut()
{
	difference()
	{
		 translate([-casebase-50,-casewall-50,-height]) cube([pcbwidth+casewall*2+100,pcblength+casewall*2+100,height*2]);
		 top_cut();
	}
}

module top_body()
{
	difference()
	{
		intersection()
		{
			solid_case();
			pcb_hulled(height);
			top_half();
		}
		if(parts_top)minkowski()
		{
			if(nohull)parts_top(part=true);
			else hull()parts_top(part=true);
			translate([0,0,margin-height])cylinder(r=margin,h=height,$fn=8);
		}
		parts_space();
	}
	intersection()
	{
		solid_case();
		parts_top(block=true);
	}
}

module top_edge()
{
	difference()
	{
		intersection()
		{
			case_wall();
			top_cut();
		}
		parts_space();
	}
}

module top()
{
	translate([casewall,casewall+pcblength,pcbthickness+casetop])rotate([180,0,0])
	{
		top_body();
		top_edge();
	}
}

module bottom_body()
{
	difference()
	{
		intersection()
		{
			solid_case();
			translate([0,0,-height])pcb_hulled(height);
			bottom_half();
		}
		if(parts_bottom)minkowski()
		{
			if(nohull)parts_bottom(part=true);
			else hull()parts_bottom(part=true);
			translate([0,0,-margin])cylinder(r=margin,h=height,$fn=8);
		}
		parts_space();
	}
	intersection()
	{
		solid_case();
		parts_bottom(block=true);
	}
}

module bottom_edge()
{
        difference()
        {
                intersection()
                {
                        case_wall();
                        bottom_cut();
                }
		parts_space();
        }
}

module bottom()
{
	translate([casewall,casewall,casebase])
	{
        	bottom_body();
        	bottom_edge();
	}
}
bottom(); translate([spacing,0,0])top();
