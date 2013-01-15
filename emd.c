#include <stdio.h>
#include <math.h>
#include "common.h"
#include "emd.h"


typedef enum _condition
{
	NATURAL,
	CLAMPED,
	NOTaKNOT
}condition;

typedef struct _coefficient
{
	float a3;
	float b2;
	float c1;
	float d0;
}coefficient;

typedef struct _point
{
	coefficient *coe;
	float *xCoordinate;
	float *yCoordinate;
	float f0;
	float fn;
	int num;
	condition con;
}point;

typedef struct _comp  // ¸´Êı½á¹¹ÌåÉùÃ÷£»
{
	float re;
	float im;
}comp;

#define N CALCULATE_SIZE
#define PI 3.141592653
#define freq 25
#define peri 20.48

comp asp[N],hea[N]; //  ´æ´¢µÄºôÎüºÍĞÄÌøÖØ¹¹ĞÅºÅ£»
float breath,heart;
static coefficient coe_max[256], coe_min[256];
static float temp_x_max[256], temp_y_max[256], temp_x_min[256], temp_y_min[256];
static float m[N];
static comp ht[N];


int extr_min(float* y, int* b);
int extr_max(float* y, int* b);
void bitrev(comp data[N]);
void fft(comp data[N]);
int spline(point *point);
float func(float a,float b,float c,float d,float x,float y);
int imf(float x[N],float y[N],int b[N],float h[N]);
void emd(float *x, float *y, float fc);
float max_val(comp *biosig);

/***************************¼«Ğ¡Öµ***************************/

int extr_min(float* y, int* b)
{
	int j = 0;
	int i;

	//ÕÒµ½ËùÓĞµÄ¼«Ğ¡Öµ£¬²¢ÇÒ½«¶ÔÓ¦µÄºá×ø±ê´æÈëÊı×éÖĞ£»
	for(i=1; i<N-1; i++)
	{
		if(y[i]<y[i+1]&&y[i]<y[i-1])
		{	
			b[++j] = i;//´ÓµÚÒ»¸ö¿ªÊ¼£¬µÚÁã¸ö×¼±¸´æÔ­Ê¼ĞÅºÅµÄµÚÒ»¸öÊı£»
		}
	}
	return j;  
}

/***************************¼«´óÖµ***************************/
int extr_max(float* y, int* b)
{
	int j = 0; 
	int i;

	//ÕÒµ½ËùÓĞµÄ¼«´óÖµ£¬²¢ÇÒ½«¶ÔÓ¦µÄºá×ø±ê´æÈëÊı×éÖĞ£»
	for(i=1; i<N-1; i++)
	{
		if(y[i]>y[i+1]&&y[i]>y[i-1])
		{	
			b[++j] = i;//´ÓµÚÒ»¸ö¿ªÊ¼£¬µÚÁã¸ö×¼±¸´æÔ­Ê¼ĞÅºÅµÄµÚÒ»¸öÊı£»
		}
	}
	return j; 
}

void bitrev(comp data[N])  //fftµÄµûĞÎÔËËãÖ®Ç°ÏÈ½øĞĞÅÅĞò£»

{

    int    p=1, q, i;
    int    bit_rev[ N ];
    float   xx_r[ N ];
   
    bit_rev[ 0 ] = 0;

    while( p < N )

    {

       for(q=0; q<p; q++) 
       {

           bit_rev[ q ]     = bit_rev[ q ] * 2;

           bit_rev[ q + p ] = bit_rev[ q ] + 1;

       }

       p *= 2;

    }

	for(i=0; i<N; i++)   xx_r[ i ] = data[i].re;   
	for(i=0; i<N; i++)   data[i].re = xx_r[ bit_rev[i] ]; //xx_r[N]Òª¶¨ÒåÈ«¾Ö±äÁ¿£»

}

void fft(comp data[N])
{
	int L;												  //µúĞÎÔËËãµÄÃ¿¼¶ L = 1, 2,......,M

	int J;												  //J = 0, 1, 2....., 2^(L - 1) - 1

	int B;												  //B = 2^(L - 1)

	int P;												  //P = J * 2^(M - L)

	int M = 9; //  MµÄÖµÈ¡¾öÓÚN£¬M = log2(N);

	int k;

	float datak_re,datak_im,datakb_re,datakb_im;	

	bitrev(data);   //ÅÅĞò£»

    //µúĞÎÔËËã;													
	for(L = 1; L <= M; L++)
	{
		B = 1 << (L - 1);
		
		for(J = 0; J <= B - 1; J++)
		{
			P = J * (1 << (M - L));
			
			for(k = J; k <= N - 1; k += (1 << L))
			{	datak_re = data[k].re;
				datak_im = data[k].im;
				datakb_re = data[k+B].re;
				datakb_im = data[k+B].im;
				data[k].re = datak_re + cos(2*PI*P/N)*datakb_re + sin(2*PI*P/N)*datakb_im;
				data[k].im = datak_im + cos(2*PI*P/N)*datakb_im - sin(2*PI*P/N)*datakb_re;
				data[k+B].re = datak_re - cos(2*PI*P/N)*datakb_re - sin(2*PI*P/N)*datakb_im;
				data[k+B].im = datak_im - cos(2*PI*P/N)*datakb_im + sin(2*PI*P/N)*datakb_re;
			}

		}

	}
}

/**********************************spline********************************************/

int spline(point *point)
{
	float *x = (*point).xCoordinate, *y = (*point).yCoordinate;
	int n = (*point).num - 1;
	coefficient *coe = (*point).coe;
	condition con = (*point).con;
	float temp;
	int i;
    float h[256], d[256], a[256], b[256], c[256], f[256];
    float	*m = b;
	/* ¼ÆËã h[] d[] */
	for (i = 0; i < n; i++)
	{
		h[i] = x[i + 1] - x[i];
		d[i] = (y[i + 1] - y[i]) / h[i];
	}
	/**********************
	** ³õÊ¼»¯ÏµÊıÔö¹ã¾ØÕó
	**********************/
	a[0] = (*point).f0;
	c[n] = (*point).fn;
	/* ¼ÆËã a[] b[] d[] f[] */
	switch(con)
	{
		case NATURAL:
			f[0] = a[0];
			f[n] = c[n];
			a[0] = 0;
			c[n] = 0;
			c[0] = 0;
			a[n] = 0;
			b[0] = 1;
			b[n] = 1;
		break;
		case CLAMPED:
			f[0] = 6 * (d[0] - a[0]);
			f[n] = 6 * (c[n] - d[n - 1]);
			a[0] = 0;
			c[n] = 0;
			c[0] = h[0];
			a[n] = h[n - 1];
			b[0] = 2 * h[0];
			b[n] = 2 * h[n - 1];
		break;
		case NOTaKNOT:
			f[0] = 0;
			f[n] = 0;
			a[0] = h[0];
			c[n] = h[n - 1];
			c[0] = -(h[0] + h[1]);
			a[n] = -(h[n - 2] + h[n - 1]);
			b[0] = h[1];
			b[n] = h[n - 2];
		break;
	}
	for (i = 1; i < n; i++)
	{
		a[i] = h[i - 1];
		b[i] = 2 * (h[i - 1] + h[i]);
		c[i] = h[i];
		f[i] = 6 * (d[i] - d[i - 1]);
	}
	/*************
    ** ¸ßË¹ÏûÔª
    *************/
    /* µÚ0ĞĞµ½µÚ(n-3)ĞĞµÄÏûÔª */
    for(i = 0; i <= n - 3; i++)
    {
        /* Ñ¡Ö÷Ôª */
        if( fabs(a[i + 1]) > fabs(b[i]) )
        {
            temp = a[i + 1]; a[i + 1] = b[i]; b[i] = temp;
            temp = b[i + 1]; b[i + 1] = c[i]; c[i] = temp;
            temp = c[i + 1]; c[i + 1] = a[i]; a[i] = temp;
            temp = f[i + 1]; f[i + 1] = f[i]; f[i] = temp;
        }
        temp = a[i + 1] / b[i];
        a[i + 1] = 0;
        b[i + 1] = b[i + 1] - temp * c[i];
        c[i + 1] = c[i + 1] - temp * a[i];
        f[i + 1] = f[i + 1] - temp * f[i];
    }
    
    /* ×îºó3ĞĞµÄÏûÔª */
    /* Ñ¡Ö÷Ôª */
    if( fabs(a[n - 1]) > fabs(b[n - 2]) )
    {
        temp = a[n - 1]; a[n - 1] = b[n - 2]; b[n - 2] = temp;
        temp = b[n - 1]; b[n - 1] = c[n - 2]; c[n - 2] = temp;
        temp = c[n - 1]; c[n - 1] = a[n - 2]; a[n - 2] = temp;
        temp = f[n - 1]; f[n - 1] = f[n - 2]; f[n - 2] = temp;
    }
    /* Ñ¡Ö÷Ôª */
    if( fabs(c[n]) > fabs(b[n - 2]) )
    {
        temp = c[n]; c[n] = b[n - 2]; b[n - 2] = temp;
        temp = a[n]; a[n] = c[n - 2]; c[n - 2] = temp;
        temp = b[n]; b[n] = a[n - 2]; a[n - 2] = temp;
        temp = f[n]; f[n] = f[n - 2]; f[n - 2] = temp;
    }
    /* µÚ(n-1)ĞĞÏûÔª */
    temp = a[n - 1] / b[n - 2];
    a[n - 1] = 0;
    b[n - 1] = b[n - 1] - temp * c[n - 2];
    c[n - 1] = c[n - 1] - temp * a[n - 2];
    f[n - 1] = f[n - 1] - temp * f[n - 2];
    /* µÚnĞĞÏûÔª */
    temp = c[n] / b[n - 2];
    c[n] = 0;
    a[n] = a[n] - temp * c[n - 2];
    b[n] = b[n] - temp * a[n - 2];
    f[n] = f[n] - temp * f[n - 2];
    /* Ñ¡Ö÷Ôª */
    if( fabs(a[n]) > fabs(b[n - 1]) )
    {
        temp = a[n]; a[n] = b[n - 1]; b[n - 1] = temp;
        temp = b[n]; b[n] = c[n - 1]; c[n - 1] = temp;
        temp = f[n]; f[n] = f[n - 1]; f[n - 1] = temp;
    }
    /* ×îºóÒ»´ÎÏûÔª */
    temp = a[n] / b[n-1];
    a[n] = 0;
    b[n] = b[n] - temp * c[n - 1];
    f[n] = f[n] - temp * f[n - 1];
    
    /* »Ø´úÇó½â m[] */
    m[n] = f[n] / b[n];
    m[n - 1] = (f[n - 1] - c[n - 1] * m[n]) / b[n-1];
    for(i = n - 2; i >= 0; i--)
    {
        m[i] = ( f[i] - (m[i + 2] * a[i] + m[i + 1] * c[i]) ) / b[i];
    }
    /************
    ** ·ÅÖÃ²ÎÊı
    ************/
    for(i = 0; i < n; i++)
    {
        coe[i].a3 = (m[i + 1] - m[i]) / (6 * h[i]);
        coe[i].b2 = m[i] / 2;
        coe[i].c1 = d[i] - (h[i] / 6) * (2 * m[i] + m[i + 1]);
        coe[i].d0 = y[i];
    }
    return n + 1;
}

/***********************************IMF********************************************/
//Èı´ÎÑùÌõ²åÖµº¯ÊıµÄ±í´ïÊ½£»
float func(float a,float b,float c,float d,float x,float y)
{
	float z = a * pow((x - y),3) + b * pow((x - y),2) + c * (x - y) + d;
	return z;
}
int imf(float x[N],float y[N],int b[N],float h[N]) //  ÊäÈë£ºx£¬y£¬b£» Êä³ö£ºÊÇ·ñ¼ÌĞøµÄÌõ¼ş£»²úÉú£ºimf·ÖÁ¿£»
{
	int i,j;
	int n = 0;
	int flag;
	int count = 0;
	point p_max;
	point p_min;
    
	for(i=0; i<N; i++)
	{
		h[i] = y[i];
	}
	/***************
	**ÒòÎªÊÇ²É¼¯Ò»¶¨µÄµãÖ®ºóÔÙ×öEMD£¬¶øÇÒÃ¿´Î×öÍêEMDÖ®ºó¾ÍÒª½øĞĞFFTÀ´È·¶¨ÄÄĞ©·ÖÁ¿ÊÇ¸úºôÎü»òÕßĞÄÌøÓĞÓÃµÄÆµÂÊ·ÖÁ¿£¬ËùÒÔ²ÉÑùµÄµãÊıºÍÆµÂÊÒªºÍFFT
	**µÄÏàÍ¬¡£ËùÒÔÒª²É¼¯512¸öµã¡£µ«ÊÇ´Ë³ÌĞòÊÇ²âÊÔ³ÌĞò£¬ÏÈ½«µãµÄÊıÄ¿¼õÉÙ¡£
	**µÚÒ»²½£ºÄ£Äâ²úÉúÔ­Ê¼ĞÅºÅµÄºá×İ×ø±ê£»
	**ÒòÎªÒ»´Îµü´ú²»Ò»¶¨¾ÍÇó³öÁË×î¸ßÆµ£¬ËùÒÔÒª·´¸´µü´úÖ±µ½×îºóµÄÆ½¾ù°üÂçÎªÁã£»
	****************/
	do
	{
		flag = 0; //ÉèÖÃ±êÖ¾Î»£¬¿ØÖÆÑ­»·µÄÌõ¼ş£»
	    //µÚ¶ş²½£ºÕÒ¼«´óÖµµã£¬¶Ô¼«´óÖµµã½øĞĞ°üÂç£¬Çó³öÃ¿Á½¸ö¼«´óÖµÇø¼äµÄ°üÂçº¯Êı£»
		n = extr_max(h,b);   //¼«´óÖµµãµÄ¸öÊı£»

		if(n < 2)
		return 1; //ÅĞ¶Ï¼«ÖµµãÊÇ·ñ´óÓÚµÈÓÚ¶ş,Èç¹û²»Âú×ãÌõ¼ş£¬¾ÍÖ±½ÓÍË³ö£»

		b[0] = 0;
		b[n+1] = N-1; 
		n = n+2; //Õâ¾ä»°µÄÒâË¼ÊÇ£¬ÒªÇóÕû¸ö¯ÊıµÄ°üÂç£¬Òª¼ÓÉÏ¿ªÍ·ºÍ½áÎ²µÄµã£¬¹Ê±È¼«´óÖµ¶àÁ½¸öµã¡£

		for(i=0; i<n; i++)
		{
			temp_x_max[i] = x[b[i]];
			temp_y_max[i] = h[b[i]];
		}
		p_max.xCoordinate = temp_x_max;
		p_max.yCoordinate = temp_y_max;
		p_max.f0 = 0;
		p_max.fn = 0;
		p_max.num = n;
		p_max.con = NOTaKNOT;
		p_max.coe = coe_max;
		spline(&p_max);
		j = n - 2;
		for(i=N-1; i>=0; i--)
		{
			if(x[i] >= x[b[j]])
			{
				m[i] = func(coe_max[j].a3,coe_max[j].b2,coe_max[j].c1,coe_max[j].d0,x[i],x[b[j]])/2;
			}
			else
			{
				j--;
				m[i] = func(coe_max[j].a3,coe_max[j].b2,coe_max[j].c1,coe_max[j].d0,x[i],x[b[j]])/2;
			}
		}

	   //µÚÎå½£ºÕÒ¼«Ğ¡Öµµã£¬¶Ô¼«Ğ¡Öµµã½øĞĞ°üÂç£¬Çó³öÃ¿Á½¸ö¼«Ğ¡ÖµÇø¼äµÄ°üÂçº¯Êı£»
		n = extr_min(h,b);   //¼«Ğ¡ÖµµãµÄ¸öÊı£»

		if(n < 2)
		return 1; //ÅĞ¶Ï¼«ÖµµãÊÇ·ñ´óÓÚµÈÓÚ¶ş,Èç¹û²»Âú×ãÌõ¼ş£¬¾ÍÖ±½ÓÍË³ö£»

		b[0] = 0;
		b[n+1] = N-1;
		n = n+2; //Õâ¾ä»°µÄÒâË¼ÊÇ£¬ÒªÇóÕû¸öº¯ÊıµÄ°üÂç£¬Òª¼ÓÉÏ¿ªÍ·ºÍ½áÎ²µÄµã£¬¹Ê±È¼«Ğ¡Öµ¶àÁ½¸öµã¡£
	
		for(i=0; i<n; i++)
		{
			temp_x_min[i] = x[b[i]];
			temp_y_min[i] = h[b[i]];
		}
		p_min.xCoordinate = temp_x_min;
		p_min.yCoordinate = temp_y_min;
		p_min.f0 = 0;
		p_min.fn = 0;
		p_min.num = n;
		p_min.con = NOTaKNOT;
		p_min.coe = coe_min;
		spline(&p_min);
	
	//Çó³öÕûÌåµÄ¶ş·ÖÖ®Ò»£¬¼´°üÀ¨¼«´óÖµµã¡£
		j = n - 2; //jÖØĞÂÖÃÊı ÕâµãºÜÖØÒªµÄ£¡£¡
		for(i=N-1; i>=0; i--)
		{
			if(x[i] >= x[b[j]])
				m[i] += func(coe_min[j].a3,coe_min[j].b2,coe_min[j].c1,coe_min[j].d0,x[i],x[b[j]])/2;
			else
			{
				j--;
				m[i] += func(coe_min[j].a3,coe_min[j].b2,coe_min[j].c1,coe_min[j].d0,x[i],x[b[j]])/2;
			}
		}

		for(i=0; i<N; i++)
		{
				if(m[i] >= 0.1) // ²»ÖªµÀÅĞ¶¨Ìõ¼şÊÇ²»ÊÇ¹ıÓÚÑÏ½÷£¬ÓĞËµÓÃÏµÊıµÄ£¬¿´Çé¿ö£»
				flag = 1;
		}

  	    //ÓÃÔ­Ê¼µÄĞÅºÅ¼õÈ¥ĞÂ²úÉúµÄ°üÂçÆ½¾ùĞÅºÅ£»
		for(i = 0; i< N; i++)
		{
			h[i] = h[i] - m[i]; 
		}
		//ÉèÖÃ±êÖ¾Î»£¬ÅĞ¶ÏÆ½¾ù°üÂçĞÅºÅÊÇ·ñÓëxÖáÖØºÏ£»
		count++;
 	if(count == 40)
 		flag = 0;
	}while(flag);  // 
	return 0;
}

void emd(float *x, float *y, float fc)//fcÎª¶Ô¸ø½øµÄ³õÊ¼ĞÅºÅ½øĞĞfftÖ®ºóÇóµÃµÄÆµÆ××î´óÖµ¶ÔÓ¦µÄÆµÂÊ
{
	
	int go_on;	
	int count1 = 0;
	int count2 = 0;
	int i = 0;
	int j = 0;
	float total = 0;
	float e_asp = 0;
	float e_hea = 0;
	comp data[N]; //ÉêÇëFFT±ä»»µÄ¾²Ì¬ÄÚ´æ£»
    float h[N];  //´æ´¢yµÄÖµ£¬±£Ö¤yµÄÖµ²»±»ÆÆ»µ£»
	int b[N];
    float im[10][512];
    int num = 0;
	//FILE * fp;
	for(i=0; i<N; i++)
	{
		h[i] = y[i];
	}

	//´æ´¢IMF·ÖÁ¿Öµ£»
   
	do
	{   
		go_on = imf(x,h,b,im[num]);
		for(j=0; j<N; j++)
		{
			h[j] = h[j] - im[num][j];
		}
		num++;
	}while(!go_on);
//	/*
//ÎÄ¼ş²Ù×÷(1)
/*	fp = fopen("D:\\imf.txt","w");
	fprintf(fp,"###################\nThe number of imf is %d\n",count);
	//´òÓ¡Ã¿¸öimf·ÖÁ¿µÄÊıÖµ£»
	for(i=0; i<count; i++)
	{
		fprintf(fp,"\nIMF %d:\n",i);
		for(j=0; j<N; j++)
			fprintf(fp,"%f,",im[i][j]);
		fprintf(fp,"**************************\n");
	}
	fclose(fp);*/
//ÎÄ¼ş²Ù×÷½áÊø(1)
// */
	//ÒÔÉÏ¾ÍÊÇ·Ö½â³öcount¸öimf·ÖÁ¿
	//*****************************************************************************************

	//ÒÔÏÂÒª¶Ô·Ö½â³öµÄÃ¿¸öimf·ÖÁ¿½øĞĞFFT£¬È»ºó¶ÔÄÜÁ¿½øĞĞ»ı·Ö£¬Çó³ö¶ÔºôÎü»òÕßĞÄÌøÓĞÓÃµÄÆµÂÊ·ÖÁ¿½øĞĞµş¼Ó£»
	//³õÊ¼ÖµÎªÁã£»
	for(i=0;i<N;i++)
	{
		asp[i].re = 0;
		asp[i].im = 0; 
		hea[i].re = 0;
		hea[i].im = 0;
	}
	for(i=0; i<num; i++)
	{
		for(j=0; j<N; j++)
		{
			data[j].re = im[i][j];
			data[j].im = 0;
		}
		//FFT;
		fft(data);
		//Ã¿¸öimfµÄÆµÆ×¼ÆËãÖ®Ç°£¬Òª°ÑÆµÆ×ÄÜÁ¿ÖµÇåÁã£»
		total = 0;
		e_asp = 0;
		e_hea = 0;
		//Çó³öÃ¿¸öimfÖĞºôÎüºÍĞÄÌøÄÜÁ¿µÄ°Ù·Ö±È£¬À´ÖØ¹¹ºôÎüºÍĞÄÌøĞÅºÅ£»
		for(j=0; j<N/2; j++)   //ÆµÆ×½üËÆÊÇ¶Ô³ÆµÄ£¬ËùÒÔÈ¡Ç°Ò»°ë£¬¾ÍÊÇÆµÂÊĞ¡ÓÚ12.8HzµÄ£»
		{
			total += pow(data[j].re, 2) ; //ºôÎüÇø¼ä£»

			if((float)freq*j/(N-1) >= (fc - 0.2) && (float)freq*j/(N-1) <= (fc + 0.2))
				e_asp += pow(data[j].re, 2) ;

			else if((float)freq*j/(N-1) >= (fc + 0.5) && (float)freq*j/(N-1) <= 3) //ĞÄÌøÇø¼ä£» //Ó¦¸ÃÊÇ1ºÍ3£¬ÔİÊ±¸Ä¶¯£¬·ÂÕæÓÃ£»
				e_hea += pow(data[j].re, 2) ;
		}
		if(e_asp/total >= 0.5)
		{
			count1++;
			for(j=0; j<N; j++)
			{
				asp[j].re += im[i][j];
			}
		}
		if(e_hea/total >= 0.5)
		{
			count2++;
			for(j=0; j<N; j++)
			{
				hea[j].re += im[i][j];
			}
		}
	}
//	/*
//ÎÄ¼ş²Ù×÷(2)£»
	/* fp=fopen("D:\\asp_hea.txt","w");
	 fprintf(fp,"**************************************************\n");
	 fprintf(fp,"count = %d\ncount1 = %d\ncount2 = %d\n",count,count1,count2);
	 fprintf(fp,"The aspiration value is:\n");
	 for(j=0; j<N; j++)
	 {
		fprintf(fp,"%f,",asp[j]);
	 }
	 fprintf(fp,"\n\nThe heartbeat value is:\n");
	 for(j=0; j<N; j++)
	 {
		fprintf(fp,"%f,",hea[j]);
	 }
	 fclose(fp);*/

}
//ÇóºôÎüÂÊºÍĞÄÂÊµÄ×î´óÖµ²¢·µ»ØÆä¶ÔÓ¦µÄÆµÂÊ£»

float max_val(comp *biosig)
{
	int i,max;
	max = 1;
	for(i=2; i<N/2; i++)
	{
		if(pow(biosig[max].re, 2) + pow(biosig[max].im, 2) < pow(biosig[i].re, 2) + pow(biosig[i].im, 2))
			max = i;
	}
	return (float)max*freq/N;
}

float g_x[N];


void emd_init()
{
	int i;
	for(i=0;i<N;i++)
	{
	  g_x[i]= (float)i* peri/(N-1);
	}	
}
//¼ÆËãºôÎüÂÊºÍĞÄÂÊµÄº¯Êı
void calculate(float* voltage, calc_result* result)
{
	//FILE *fp;
	int i;
	float fcc;
    
	for(i=0; i<N; i++)  // h[i]»ñÈ¡yµÄÖµ£¬À´½øĞĞµÚÒ»´ÎµÄfft£»
	{
		ht[i].re = voltage[i];
		ht[i].im = 0;
	}
//ÏÈ¶Ô³õÊ¼ĞÅºÅ½øĞĞfft£¬Çó³öºôÎüÂÊµÄ¹À¼ÆÖµ£»
	fft(ht);
	fcc = max_val(ht);
  	log(INFO, "%f\n",fcc);
	emd(g_x,voltage,fcc);
	
	for (i = 0; i < N; i++) {
		result->raw[i] = voltage[i];
		result->asp[i] = asp[i].re;
		result->hea[i] = hea[i].re;	
	}
	
	fft(asp);
	fft(hea);
//ÎÄ¼şĞ´Èë(3)
//	/*
/*	fp=fopen("D:\\spectrum.txt","w");

	fprintf(fp,"The original value after fft is:\n");
	for(i=0;i<N;i++)
	{
		fprintf(fp,"%f+%fi,",ht[i].re,ht[i].im);
	}

	fprintf(fp,"\n\nThe aspiration value after fft is:\n");
	for(i=0;i<N;i++)
	{
		fprintf(fp,"%f+%fi,",asp[i].re,asp[i].im);
	}
	fprintf(fp,"\n\nThe heartbeat value after fft is:\n");
	for(i=0;i<N;i++)
	{
		fprintf(fp,"%f+%fi,",hea[i].re,hea[i].im);
	}
	fclose(fp);
//ÎÄ¼şĞ´Èë½áÊø(3)
// */
	breath=max_val(asp);
	heart=max_val(hea);
		
	log(INFO, "The aspiration_rate is %fHz\n",breath);
	log(INFO, "The heartbeat_rate is %fHz\n",heart);
	
	result->asp_rate = breath;
	result->hea_rate = heart;
}






















