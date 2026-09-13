ratRailParticle
{
	nopicmip
	cull disable
	{
		map gfx/misc/ratrail/particle.tga
		blendfunc add
		rgbGen entity
	}
}
ratRailSpiral1
{
	nopicmip
	sort nearest
	cull disable
	{
		map gfx/misc/ratrail/railspiral1.tga
		blendfunc add
		rgbGen entity
	}
	//{
	//	clampmap gfx/misc/ratrail/railspiral.tga
	//	blendfunc add
	//	rgbGen entity
	//	//rgbGen const ( 0.5 0.5 0.5 )
	//	//alphaGen entity
	//}
}
ratRailSpiral
{
	nopicmip
	sort nearest
	cull disable
	{
		clampmap gfx/misc/ratrail/railspiral.tga
		tcMod transform 10 0 0 1 0 0
		blendfunc add
		rgbGen entity
	}
}
//ratRailCoreBeefy
ratRailCore
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 64 gfx/misc/ratrail/beefyrail/rail1.tga  gfx/misc/ratrail/beefyrail/rail2.tga  gfx/misc/ratrail/beefyrail/rail3.tga  gfx/misc/ratrail/beefyrail/rail4.tga  gfx/misc/ratrail/beefyrail/rail5.tga  gfx/misc/ratrail/beefyrail/rail6.tga  gfx/misc/ratrail/beefyrail/rail7.tga  

		// pattern repeats more often, higher quality 
		////tcMod transform 2 0 0 1 0 0
		// scaled normally
		//tcMod transform 1 0 0 1 0 0
		// pattern repeats more often, higher quality
		// + rail is twice as thick
		//tcMod transform 2 0 0 0.5 0 0.25
		// pattern repeats more often, higher quality
		// + rail thicker by a factor of 4/3
		tcMod transform 2 0 0 0.75 0 0.125

		blendfunc add
		rgbGen entity
	}
}
//ratRailCoreBeefyOverlay
ratRailCoreOverlay
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 40 gfx/misc/ratrail/beefyrail/railoverlay1.tga  gfx/misc/ratrail/beefyrail/railoverlay2.tga  gfx/misc/ratrail/beefyrail/railoverlay3.tga  gfx/misc/ratrail/beefyrail/railoverlay4.tga  gfx/misc/ratrail/beefyrail/railoverlay5.tga  gfx/misc/ratrail/beefyrail/railoverlay6.tga  gfx/misc/ratrail/beefyrail/railoverlay7.tga  

		// pattern repeats more often, higher quality 
		//tcMod transform 2 0 0 1 0 0
		// scaled normally
		//tcMod transform 1 0 0 1 0 0
		// pattern repeats more often, higher quality
		// + rail is twice as thick
		//tcMod transform 2 0 0 0.5 0 0.25
		// pattern repeats more often, higher quality
		// + rail thicker by a factor of 4/3
		tcMod transform 2 0 0 0.75 0 0.125

		blendfunc add
		rgbGen entity
	}
}

//ratRailCore
//{
//	nopicmip
//	sort nearest
//	cull disable
//	{
//		animmap 64 gfx/misc/ratrail/rail1.tga  gfx/misc/ratrail/rail2.tga  gfx/misc/ratrail/rail3.tga  gfx/misc/ratrail/rail4.tga  gfx/misc/ratrail/rail5.tga  gfx/misc/ratrail/rail6.tga  gfx/misc/ratrail/rail7.tga  
//
//		// pattern repeats more often, higher quality 
//		tcMod transform 2 0 0 1 0 0
//		// scaled normally
//		//tcMod transform 1 0 0 1 0 0
//
//		blendfunc add
//		rgbGen entity
//	}
//}
//ratRailCoreOverlay
//{
//	nopicmip
//	sort nearest
//	cull disable
//	{
//		animmap 40 gfx/misc/ratrail/railoverlay1.tga  gfx/misc/ratrail/railoverlay2.tga  gfx/misc/ratrail/railoverlay3.tga  gfx/misc/ratrail/railoverlay4.tga  gfx/misc/ratrail/railoverlay5.tga  gfx/misc/ratrail/railoverlay6.tga  gfx/misc/ratrail/railoverlay7.tga  
//
//		// pattern repeats more often, higher quality 
//		tcMod transform 2 0 0 1 0 0
//		// scaled normally
//		//tcMod transform 1 0 0 1 0 0
//
//		blendfunc add
//		rgbGen entity
//	}
//}

ratRailCoreFat
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 40 gfx/misc/ratrailfat/rail1.tga  gfx/misc/ratrailfat/rail2.tga  gfx/misc/ratrailfat/rail3.tga  gfx/misc/ratrailfat/rail4.tga  gfx/misc/ratrailfat/rail5.tga  gfx/misc/ratrailfat/rail6.tga  gfx/misc/ratrailfat/rail7.tga  
		blendfunc add
		rgbGen entity
	}
}

ratRailCoreFat
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 40 gfx/misc/ratrailfat/rail1.tga  gfx/misc/ratrailfat/rail2.tga  gfx/misc/ratrailfat/rail3.tga  gfx/misc/ratrailfat/rail4.tga  gfx/misc/ratrailfat/rail5.tga  gfx/misc/ratrailfat/rail6.tga  gfx/misc/ratrailfat/rail7.tga  
		blendfunc add
		rgbGen entity
	}
}
ratRailCoreOverlayFat
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 24 gfx/misc/ratrailfat/railoverlay1.tga  gfx/misc/ratrailfat/railoverlay2.tga  gfx/misc/ratrailfat/railoverlay3.tga  gfx/misc/ratrailfat/railoverlay4.tga  gfx/misc/ratrailfat/railoverlay5.tga  gfx/misc/ratrailfat/railoverlay6.tga  gfx/misc/ratrailfat/railoverlay7.tga  
		blendfunc add
		rgbGen entity
	}
}

//ratRailCore1
//{
//	sort nearest
//	cull disable
//	{
//		animmap 24 gfx/misc/ratrail/rail1.tga  gfx/misc/ratrail/rail2.tga  gfx/misc/ratrail/rail3.tga  gfx/misc/ratrail/rail4.tga  gfx/misc/ratrail/rail5.tga  gfx/misc/ratrail/rail6.tga  gfx/misc/ratrail/rail7.tga  
//		blendfunc add
//		rgbGen entity
//	}
//	{
//		animmap 24 gfx/misc/ratrail/railoverlay1.tga  gfx/misc/ratrail/railoverlay2.tga  gfx/misc/ratrail/railoverlay3.tga  gfx/misc/ratrail/railoverlay4.tga  gfx/misc/ratrail/railoverlay5.tga  gfx/misc/ratrail/railoverlay6.tga  gfx/misc/ratrail/railoverlay7.tga  
//		blendfunc add
//		rgbGen identity
//	}
//}
//
