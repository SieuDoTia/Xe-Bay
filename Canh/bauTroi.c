// (1920 × 1080)/(1280 × 720) = (2 073 6000)/(921 600) = 2.25 
//  Làm ảnh bầu trời cho xài với phần mềm Blender
//  Phiên Bản 0.65
//  Phát hành 2560/10/26
//  Khởi đầu 2560-02-25

//  Biên dịch cho gcc: gcc -lm -lz doTia.c -o <tên chương trình>
//  Biên dịch cho clang: clang -lm -lz doTia.c -o <tên chương trình>


// (0,0; 0,25)             (1,0; 0,25)
//  +---------------------------+
//  |                           |
//  |                           |
//  +---------------------------+
//  |                           |
//  |                           |
//  +---------------------------+
// (0,0; –0,25)            (1,0; –0,25)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <zlib.h>

#define kKIEU_HALF 1
#define kKIEU_FLOAT 2

/* Màu */
typedef struct {
   float d;
   float l;
   float x;
} Mau;

/* Ảnh */
typedef struct {
   unsigned short beRong;   // bề rộng
   unsigned short beCao;    // bề cao
   float coKichDiemAnh;     // cỡ kích điểm ảnh (điểm anh/đơn vị thế giới)
   float *kenhDo;      // kênh đỏ
   float *kenhLuc;     // kênh lục
   float *kenhXanh;    // kênh xanh
   float *kenhDuc;     // kênh đục
   float *kenhXa;    // kênh cách xa
} Anh;


/* Môi Trường */
typedef struct {
   Mau mauDayTroi;             // đấy trời
   Mau mauGiuaTroiDuoi;        // màu giữa trời dượi
   Mau mauChanTroi;            // màu chân trời
   Mau mauGiuaTroiTren;        // màu giữa trời trên
   Mau mauDinhTroi;            // màu đỉnh trời
   float viTriMauGiuaTren;   // phân số nữa bầu trời cho vị trí màu giữa trời
   float viTriMauGiuaDuoi;   // phân số nữa bầu trời cho vị trí màu giữa trời

   
   unsigned char coMatTroi; // có mặt trời - đêm không mặt trời
   Mau mauMatTroi;          // màu mặt trời
   float kinhTuyenMatTroi;  // kinh tuyến mặt trời
   float viTuyenMatTroi;    // vĩ tuyến mặt trời
   float banKinhMatTroi;    // bán kính mặt trời
   float banKinhHaoQuang;   // bán kính hào quang
   
   Mau mauToi;           // màu tối - cho hướng ngược chiều mặt trời
} MoiTruong;

/* Bầu Trời */
void bauTroi( Anh *anhBauTroi, MoiTruong *moiTruong );

/*  Mặt Trời */
void matTroi( Anh *sanhBauTroi, MoiTruong *moiTruong );

void mauToiNghichMatTroi( Anh *anhBauTroi, MoiTruong *moiTruong );

/* Các bộ màu bầu trời */
MoiTruong bauTroiVang();
MoiTruong bauTroiChieu_xeBay();
MoiTruong bauTroiToi_xeBay();


// ---- lưu ảnh RLE
void luuAnhZIP( char *tenTep, Anh *anh, unsigned char kieuDuLieu, unsigned short thoiGianKetXuat );   // lưu ảnh ZIP

Anh taoAnhVoiCoKich( unsigned short beRong, unsigned short beCao, float coKichDiemAnh );
void xoaAnh( Anh *anh );


int main( int argc, char **arg ) {

   unsigned short beRong = 3072;
   unsigned short beCao = beRong >> 1;
   Anh anhBauTroi = taoAnhVoiCoKich( beRong, beCao, 1.0f );

 //  if( argc > 0 )
   MoiTruong moiTruong = bauTroiToi_xeBay();
   char tenAnh[] = "BauTroi00.exr";

   // ---- bầu trời
   bauTroi( &anhBauTroi, &moiTruong );
   
   if( moiTruong.coMatTroi ) {
      matTroi( &anhBauTroi, &moiTruong );
      mauToiNghichMatTroi( &anhBauTroi, &moiTruong );
   }
   
   luuAnhZIP( tenAnh, &anhBauTroi, kKIEU_HALF, 0 );
   xoaAnh( &anhBauTroi );
   
   printf( "%d %d điểm ảnh\n", beRong, beCao );
   return 1;
}

/* Hàm để tính ảnh bầu trời */
//  +------------------------------------+ đỉnh trời    (0,25)
//  |                                    |
//  |                                    |
//  +------------------------------------+ giữa trời trên    (vị trí giũa trời, phân số*0,25)
//  |                                    |
//  +------------------------------------+ chân trời    (0,0)
//  |                                    |
//  +------------------------------------+ giữa trời dưới
//  |                                    |
//  |                                    |
//  +------------------------------------+ đáy trời
// 0,0                                  1.0


void bauTroi( Anh *anhBauTroi, MoiTruong *moiTruong ) {

   unsigned short beRong = anhBauTroi->beRong;
   unsigned short nuaBeCao = beRong >> 2;        // (0,25)
   unsigned short giuaTroiDuoi = nuaBeCao - nuaBeCao*moiTruong->viTriMauGiuaDuoi;
   unsigned short giuaTroiTren = nuaBeCao + nuaBeCao*moiTruong->viTriMauGiuaTren;

   
   // ----
   float cach_giuaDay = nuaBeCao*(1.0f - moiTruong->viTriMauGiuaDuoi);
   float cach_giuaDinh = nuaBeCao*(1.0f - moiTruong->viTriMauGiuaTren);

   Mau buocMau_giuaDayDuoi;
   buocMau_giuaDayDuoi.d = (moiTruong->mauDayTroi.d - moiTruong->mauGiuaTroiDuoi.d)/cach_giuaDay;
   buocMau_giuaDayDuoi.l = (moiTruong->mauDayTroi.l - moiTruong->mauGiuaTroiDuoi.l)/cach_giuaDay;
   buocMau_giuaDayDuoi.x = (moiTruong->mauDayTroi.x - moiTruong->mauGiuaTroiDuoi.x)/cach_giuaDay;

   // ----
   Mau buocMau_giuaDinhTren;
   buocMau_giuaDinhTren.d = (moiTruong->mauDinhTroi.d - moiTruong->mauGiuaTroiTren.d)/cach_giuaDinh;
   buocMau_giuaDinhTren.l = (moiTruong->mauDinhTroi.l - moiTruong->mauGiuaTroiTren.l)/cach_giuaDinh;
   buocMau_giuaDinhTren.x = (moiTruong->mauDinhTroi.x - moiTruong->mauGiuaTroiTren.x)/cach_giuaDinh;

   // ----
   float cach_chanGiuaDuoi = nuaBeCao*moiTruong->viTriMauGiuaDuoi;
   Mau buocMau_chanGiuaDuoi;
   buocMau_chanGiuaDuoi.d = (moiTruong->mauGiuaTroiDuoi.d - moiTruong->mauChanTroi.d)/cach_chanGiuaDuoi;
   buocMau_chanGiuaDuoi.l = (moiTruong->mauGiuaTroiDuoi.l - moiTruong->mauChanTroi.l)/cach_chanGiuaDuoi;
   buocMau_chanGiuaDuoi.x = (moiTruong->mauGiuaTroiDuoi.x - moiTruong->mauChanTroi.x)/cach_chanGiuaDuoi;
   
   float cach_chanGiuaTren = nuaBeCao*moiTruong->viTriMauGiuaTren;
   Mau buocMau_chanGiuaTren;
   buocMau_chanGiuaTren.d = (moiTruong->mauGiuaTroiTren.d - moiTruong->mauChanTroi.d)/cach_chanGiuaTren;
   buocMau_chanGiuaTren.l = (moiTruong->mauGiuaTroiTren.l - moiTruong->mauChanTroi.l)/cach_chanGiuaTren;
   buocMau_chanGiuaTren.x = (moiTruong->mauGiuaTroiTren.x - moiTruong->mauChanTroi.x)/cach_chanGiuaTren;


   // ==== tô màu
   unsigned short hang = 0;
   while( hang < anhBauTroi->beCao ) {
      unsigned short cot = 0;
      Mau mauHang;
      if( hang < giuaTroiDuoi ) {
          mauHang.d = moiTruong->mauDayTroi.d - buocMau_giuaDayDuoi.d*hang;
          mauHang.l = moiTruong->mauDayTroi.l - buocMau_giuaDayDuoi.l*hang;
          mauHang.x = moiTruong->mauDayTroi.x - buocMau_giuaDayDuoi.x*hang;
      }
      else if( hang < nuaBeCao ) {
          unsigned short hangTuongDoi = hang - giuaTroiDuoi;
          mauHang.d = moiTruong->mauGiuaTroiDuoi.d - buocMau_chanGiuaDuoi.d*hangTuongDoi;
          mauHang.l = moiTruong->mauGiuaTroiDuoi.l - buocMau_chanGiuaDuoi.l*hangTuongDoi;
          mauHang.x = moiTruong->mauGiuaTroiDuoi.x - buocMau_chanGiuaDuoi.x*hangTuongDoi;
      }
      else if( hang < giuaTroiTren ) {
          unsigned short hangTuongDoi = hang - nuaBeCao;
          mauHang.d = moiTruong->mauChanTroi.d + buocMau_chanGiuaTren.d*hangTuongDoi;
          mauHang.l = moiTruong->mauChanTroi.l + buocMau_chanGiuaTren.l*hangTuongDoi;
          mauHang.x = moiTruong->mauChanTroi.x + buocMau_chanGiuaTren.x*hangTuongDoi;
      }
      else {
          unsigned short hangTuongDoi = hang - giuaTroiTren;
          mauHang.d = moiTruong->mauGiuaTroiTren.d + buocMau_giuaDinhTren.d*hangTuongDoi;
          mauHang.l = moiTruong->mauGiuaTroiTren.l + buocMau_giuaDinhTren.l*hangTuongDoi;
          mauHang.x = moiTruong->mauGiuaTroiTren.x + buocMau_giuaDinhTren.x*hangTuongDoi;
//printf( "%5.3f %5.3f %5.3f\n", mauHang.d, mauHang.l, mauHang.x );
      }

      while( cot < beRong ) {
          anhBauTroi->kenhDo[hang*beRong + cot] = mauHang.d;
          anhBauTroi->kenhLuc[hang*beRong + cot] = mauHang.l;
          anhBauTroi->kenhXanh[hang*beRong + cot] = mauHang.x;
          cot++;
      }
      hang++;
   }
}

#pragma mark ---- Mặt Trời
// +---------------+
// | /    ---    \ |
// |   /       \   |
// |  |    +    |  |
// |   \       /   |
// | \    ---    / |
// +---------------+
//         +-------> bán kính hào quang
//         +---->    bán kính mặt trời

void matTroi( Anh *anhBauTroi, MoiTruong *moiTruong ) {
   
   short beRong = anhBauTroi->beRong;
   
   short banKinh_matTroi = moiTruong->banKinhMatTroi*beRong;
   short banKinhBinh_matTroi = banKinh_matTroi*banKinh_matTroi;
   
   short banKinhBinh_haoQuang = moiTruong->banKinhHaoQuang*beRong;
   banKinhBinh_haoQuang *= banKinhBinh_haoQuang;
   
   float cachGiuaBanKinh = (moiTruong->banKinhHaoQuang - moiTruong->banKinhMatTroi)*beRong;

   // ---- tính vùng mặt trời
   short benTrai = (moiTruong->kinhTuyenMatTroi - moiTruong->banKinhHaoQuang)*beRong;
   short benPhai = (moiTruong->kinhTuyenMatTroi + moiTruong->banKinhHaoQuang)*beRong;
   short benDuoi = (moiTruong->viTuyenMatTroi - moiTruong->banKinhHaoQuang + 0.25f)*beRong;
   short benTren = (moiTruong->viTuyenMatTroi + moiTruong->banKinhHaoQuang + 0.25f)*beRong;
   
   short tamX = moiTruong->kinhTuyenMatTroi*beRong;
   short tamY = (moiTruong->viTuyenMatTroi + 0.25f)*beRong;   // <--------
   
   if( benTrai < 0 )
      benTrai = 0;
   if( benPhai > beRong )
      benPhai = beRong;
   if( benDuoi < 0 )
      benDuoi = 0;
   if( benTren > beRong*0.5f )
      benTren = beRong*0.5f;
   
   // ---- quét vùng để vẽ mặt trời
   short hang = benDuoi;
   
   while( hang < benTren ) {
   short cot = benTrai;
      while( cot < benPhai ) {
         
         // ---- vị trí tương đối với tâm
         short cachX = cot - tamX;
         short cachY = hang - tamY;
         
         short banKinhBinh = cachX*cachX + cachY*cachY;
//         printf( "%d < %d\n", banKinhBinh, banKinhBinh_matTroi );
         // ---- tô màu mặt trời
         if( banKinhBinh < banKinhBinh_matTroi ) {
            anhBauTroi->kenhDo[hang*beRong + cot] = moiTruong->mauMatTroi.d;
            anhBauTroi->kenhLuc[hang*beRong + cot] = moiTruong->mauMatTroi.l;
            anhBauTroi->kenhXanh[hang*beRong + cot] = moiTruong->mauMatTroi.x;
         }
         // ---- phai màu mặt trời
         else if( banKinhBinh < banKinhBinh_haoQuang ) {
            float nghichPhaiMau = 0.03f*(sqrtf(banKinhBinh) - banKinh_matTroi)/cachGiuaBanKinh + 0.97f;
            float phaiMau = 1.0f - nghichPhaiMau;
            anhBauTroi->kenhDo[hang*beRong + cot] = phaiMau*moiTruong->mauMatTroi.d
                                 + nghichPhaiMau*anhBauTroi->kenhDo[hang*beRong + cot];
            anhBauTroi->kenhLuc[hang*beRong + cot] = phaiMau*moiTruong->mauMatTroi.l
                                 + nghichPhaiMau*anhBauTroi->kenhLuc[hang*beRong + cot];
            anhBauTroi->kenhXanh[hang*beRong + cot] = phaiMau*moiTruong->mauMatTroi.x
                                 + nghichPhaiMau*anhBauTroi->kenhXanh[hang*beRong + cot];
         }
         
         cot++;
      }
      
      hang++;
   }
   
}

//
//        0,0       0,5  0,6 0,7
//        0,2        0,7  0,8 0,9
// +-------------------+----+
// |                   |    |
// |       o           |    |
// +-------------------+----+
// |                   |    |
// |                   |    |
// +-------------------+----+
//

void mauToiNghichMatTroi( Anh *anhBauTroi, MoiTruong *moiTruong ) {
   
   short beRong = anhBauTroi->beRong;
   short beCao = anhBauTroi->beRong >> 1;
   short nuaBeCao = beRong >> 2;        // (0,25)
   short kinhTuyen_matTroi = moiTruong->kinhTuyenMatTroi*anhBauTroi->beRong;
 //  printf( "kinhTuyen matTroi %d\n", kinhTuyen_matTroi );
   
   short hang = 0;
   while( hang < beCao ) {
      
      float tiSoHang = 1.0f;
      if( hang < nuaBeCao )
         tiSoHang = (float)hang/(float)nuaBeCao;
      else
         tiSoHang = (float)(beCao - hang)/(float)nuaBeCao;

      short cot = 0;
      while( cot < beRong ) {
         short cachKinhTuyen = cot - kinhTuyen_matTroi;
         if( cachKinhTuyen < 0 )
            cachKinhTuyen = -cachKinhTuyen;
         
         if( cachKinhTuyen < -beCao )
            cachKinhTuyen = beRong - cachKinhTuyen;
         else if( cachKinhTuyen > beCao ) {
            cachKinhTuyen = beRong - cachKinhTuyen;
         }
         float nghichPhaMau = 0.5f*(float)cachKinhTuyen/(float)beCao;
         nghichPhaMau *= tiSoHang;
         float phaMau = 1.0f - nghichPhaMau;
//         if( hang == nuaBeCao )
//         printf( "cot %d  cacKinh %d   nghichPha %5.3f  pha %5.3f\n", cot, cachKinhTuyen, nghichPhaMau, phaMau );
         anhBauTroi->kenhDo[hang*beRong + cot] = phaMau*anhBauTroi->kenhDo[hang*beRong + cot] + nghichPhaMau*moiTruong->mauToi.d;
         anhBauTroi->kenhLuc[hang*beRong + cot] = phaMau*anhBauTroi->kenhLuc[hang*beRong + cot] + nghichPhaMau*moiTruong->mauToi.l;
         anhBauTroi->kenhXanh[hang*beRong + cot] = phaMau*anhBauTroi->kenhXanh[hang*beRong + cot] + nghichPhaMau*moiTruong->mauToi.x;
         cot++;
      }
      hang++;
   }
}

#pragma mark ---- Màu
MoiTruong bauTroiVang() {
   
   MoiTruong moiTruong;
   
   moiTruong.mauDayTroi.d = 0.270f;
   moiTruong.mauDayTroi.l = 0.20f;
   moiTruong.mauDayTroi.x = 0.10f;
   
   moiTruong.mauGiuaTroiDuoi.d = 0.70f;
   moiTruong.mauGiuaTroiDuoi.l = 0.40f;
   moiTruong.mauGiuaTroiDuoi.x = 0.270f;
   
   moiTruong.mauChanTroi.d = 1.000f;
   moiTruong.mauChanTroi.l = 0.851f;
   moiTruong.mauChanTroi.x = 0.600f;
   
   moiTruong.mauGiuaTroiTren.d = 1.00f;
   moiTruong.mauGiuaTroiTren.l = 0.83f;
   moiTruong.mauGiuaTroiTren.x = 0.100f;
   
   moiTruong.mauDinhTroi.d = 1.000f;
   moiTruong.mauDinhTroi.l = 0.550f;
   moiTruong.mauDinhTroi.x = 0.160f;

   moiTruong.viTriMauGiuaTren = 0.15f;   // phân số nữa bầu trời cho vị trí màu gĩa trời
   moiTruong.viTriMauGiuaDuoi = 0.15f;   // phân số nữa bầu trời cho vị trí màu gĩa trời

   moiTruong.coMatTroi = 0x00;

   return moiTruong;
}

MoiTruong bauTroiChieu_xeBay() {
   
   MoiTruong moiTruong;

   moiTruong.mauDayTroi.d = 0.270f;
   moiTruong.mauDayTroi.l = 0.20f;
   moiTruong.mauDayTroi.x = 0.10f;

   moiTruong.mauGiuaTroiDuoi.d = 0.70f;
   moiTruong.mauGiuaTroiDuoi.l = 0.40f;
   moiTruong.mauGiuaTroiDuoi.x = 0.270f;

   moiTruong.mauChanTroi.d = 1.000f;
   moiTruong.mauChanTroi.l = 0.451f;
   moiTruong.mauChanTroi.x = 0.310f;
   
   moiTruong.mauGiuaTroiTren.d = 1.00f;
   moiTruong.mauGiuaTroiTren.l = 0.72f;
   moiTruong.mauGiuaTroiTren.x = 0.60f;
   
   moiTruong.mauDinhTroi.d = 0.452f;
   moiTruong.mauDinhTroi.l = 0.750f;
   moiTruong.mauDinhTroi.x = 1.000f;
   
   moiTruong.viTriMauGiuaTren = 0.15f;   // phân số nữa bầu trời cho vị trí màu gĩa trời
   moiTruong.viTriMauGiuaDuoi = 0.02f;   // phân số nữa bầu trời cho vị trí màu gĩa trời
   
   // ---- mặt trời
   moiTruong.coMatTroi = 0x01;
   moiTruong.kinhTuyenMatTroi = 0.05f;
   moiTruong.viTuyenMatTroi = 0.05f;
   moiTruong.banKinhMatTroi = 0.005f;
   moiTruong.banKinhHaoQuang = 0.035f;

   moiTruong.mauMatTroi.d = 2.375f*5.0f;
   moiTruong.mauMatTroi.l = 1.550f*5.0f;
   moiTruong.mauMatTroi.x = 1.00f*5.0f;
   
   moiTruong.mauToi.d = 0.2f;
   moiTruong.mauToi.l = 0.2f;
   moiTruong.mauToi.x = 0.5f;
   
   return moiTruong;
}

MoiTruong bauTroiToi_xeBay() {
   
   MoiTruong moiTruong;
   
   moiTruong.mauDayTroi.d = 0.125f;
   moiTruong.mauDayTroi.l = 0.073f;
   moiTruong.mauDayTroi.x = 0.076;
   
   moiTruong.mauGiuaTroiDuoi.d = 0.328f;
   moiTruong.mauGiuaTroiDuoi.l = 0.241f;
   moiTruong.mauGiuaTroiDuoi.x = 0.220f;
   
   moiTruong.mauChanTroi.d = 0.272f;
   moiTruong.mauChanTroi.l = 0.308f;
   moiTruong.mauChanTroi.x = 0.520f;
   
   moiTruong.mauGiuaTroiTren.d = 0.128f;
   moiTruong.mauGiuaTroiTren.l = 0.192f;
   moiTruong.mauGiuaTroiTren.x = 0.520f;
   
   moiTruong.mauDinhTroi.d = 0.029f;
   moiTruong.mauDinhTroi.l = 0.035f;
   moiTruong.mauDinhTroi.x = 0.168f;
   
   moiTruong.viTriMauGiuaTren = 0.15f;   // phân số nữa bầu trời cho vị trí màu gĩa trời
   moiTruong.viTriMauGiuaDuoi = 0.08f;   // phân số nữa bầu trời cho vị trí màu gĩa trời
   
   // ---- mặt trời
   moiTruong.coMatTroi = 0x00;

   return moiTruong;
}

#pragma mark ---- Ảnh
Anh taoAnhVoiCoKich( unsigned short beRong, unsigned short beCao, float coKichDiemAnh ) {
   
   Anh anh;
   anh.beRong = beRong;
   anh.beCao = beCao;
   anh.coKichDiemAnh = coKichDiemAnh;
   
   // ---- dành trí nhớ
   anh.kenhDo = malloc( sizeof( float)*beRong*beCao );
   anh.kenhLuc = malloc( sizeof( float)*beRong*beCao );
   anh.kenhXanh = malloc( sizeof( float)*beRong*beCao );
   anh.kenhDuc = malloc( sizeof( float)*beRong*beCao );
   //   float anh.kenhXa = malloc( sizeof( float)*beCao*beCao );
   
   return anh;
}

void xoaAnh( Anh *anh ) {
   
   anh->beRong = 0;
   anh->beCao = 0;
   
   free( anh->kenhDo );
   free( anh->kenhLuc );
   free( anh->kenhXanh );
   free( anh->kenhDuc );
   //   free( anh->kenhXa );
}


#pragma mark ---- Lưư Ảnh
void luuThongTinKenh_EXR( FILE *tep, unsigned char *danhSachKenh, unsigned char soLuongKenh, unsigned char kieuDuLieu );
void luuThongTinCuaSoDuLieu( FILE *tep, unsigned int beRong, unsigned int beCao );
void luuThongTinCuaSoChieu( FILE *tep, unsigned int beRong, unsigned int beCao );
void luuThoiGianKetXuat( FILE *tep, unsigned short thoiGianKetXuat );
void luuBangDuLieuAnh( FILE *tep, unsigned short soLuongThanhPhan );
void chepDuLieuKenhFloat( unsigned char *dem, const float *kenh, unsigned short beRong );
void chepDuLieuKenhHalf( unsigned char *dem, const float *kenh, unsigned short beRong );
unsigned short doiFloatSangHalf( float soFloat );
void locDuLieuTrongDem(unsigned char *dem, unsigned int beDai, unsigned char *demLoc );
unsigned int nenZIP(unsigned char *dem, int beDaiDem, unsigned char *demNen, int beDaiDemNen );

#define kSAI  0
#define kDUNG 1

/* Lưu Ảnh ZIP */
void luuAnhZIP( char *tenTep, Anh *anh, unsigned char kieuDuLieu, unsigned short thoiGianKetXuat ) {
   
   FILE *tep = fopen( tenTep, "wb" );
   //   luuThongTinKenhEXR( unsigned short beRong, unsigned short beCao );
   // ---- mã số EXR
   fputc( 0x76, tep );
   fputc( 0x2f, tep );
   fputc( 0x31, tep );
   fputc( 0x01, tep );
   
   // ---- phiên bản 2 (chỉ phiên bản 2 được phát hành)
   unsigned int phienBan = 0x02;
   fputc( 0x02, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   unsigned short beRong = anh->beRong;
   unsigned short beCao = anh->beCao;
   
   // ---- thông cho các kênh
   unsigned char danhSachKenh[4] = {'B', 'G', 'R'};
   luuThongTinKenh_EXR( tep, danhSachKenh, 3, kieuDuLieu );
   
   // ---- nén
   fprintf( tep, "compression" );
   fputc( 0x00, tep );
   fprintf( tep, "compression" );
   fputc( 0x00, tep );
   fputc( 0x01, tep );   // bề dài dữ liệu
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   fputc( 0x03, tep );  // ZIP
   
   // ---- cửa sổ dữ liệu
   luuThongTinCuaSoDuLieu( tep, beRong, beCao );
   
   // ---- cửa sổ dữ liệu
   luuThongTinCuaSoChieu( tep, beRong, beCao );
   
   // ---- thứ tự hàng
   fprintf( tep, "lineOrder" );
   fputc( 0x00, tep );
   fprintf( tep, "lineOrder" );
   fputc( 0x00, tep );
   fputc( 0x01, tep );   // bề dài dữ liệu
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   fputc( 0x00, tep );  // từ nhỏ tới lớn
   
   // ---- tỉ số cạnh điểm ảnh
   fprintf( tep, "pixelAspectRatio" );
   fputc( 0x00, tep );
   fprintf( tep, "float" );
   fputc( 0x00, tep );
   fputc( 0x04, tep );   // bề dài dữ liệu
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   fputc( 0x00, tep );  // 1.0
   fputc( 0x00, tep );
   fputc( 0x80, tep );
   fputc( 0x3f, tep );
   
   luuThoiGianKetXuat( tep, thoiGianKetXuat );
   
   // ---- trung tâm cửa sổ màn
   fprintf( tep, "screenWindowCenter" );
   fputc( 0x00, tep );
   fprintf( tep, "v2f" );
   fputc( 0x00, tep );
   fputc( 0x08, tep );   // bề dài dữ liệu
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- tọa độ x (hoành độ)
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- tọa độ y (tung độ)
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   // ---- bề rộng cửa sổ màn
   fprintf( tep, "screenWindowWidth" );
   fputc( 0x00, tep );
   fprintf( tep, "float" );
   fputc( 0x00, tep );
   fputc( 0x04, tep );   // bề dài dữ liệu
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   fputc( 0x00, tep );   // 1.0f
   fputc( 0x00, tep );
   fputc( 0x80, tep );
   fputc( 0x3f, tep );
   
   // ---- kết thúc phần đầu
   fputc( 0x00, tep );
   
   // ==== bảng cho thành phần dữ liệu ảnh
   // ---- giữ địa chỉ đầu bảng thành phần
   unsigned long long diaChiDauBangThanhPhan = ftell( tep );
   unsigned short soLuongThanhPhan = beCao >> 4;
   // ---- nếu có phần dư, cần thêm một thành phần
   if( beCao & 0xf )
      soLuongThanhPhan++;
   
   // ---- lưu bàng rỗng
   luuBangDuLieuAnh( tep, soLuongThanhPhan );
   
   unsigned long long *bangThanhPhan = malloc( soLuongThanhPhan << 3 );  // sốLượngThanhPhần * 8
   
   // ---- bề dài dệm
   unsigned int beDaiDem = (beRong << kieuDuLieu)*3 << 4; // nhân 3 cho 3 kênh, 16 hàng
   // ---- tạo đệm để lọc dữ liệu
   unsigned char *dem = malloc( beDaiDem );
   unsigned char *demLoc = malloc( beDaiDem );
   unsigned char *demNenZIP = malloc( beDaiDem << 1);  // nhân 2 cho an toàn
   unsigned short soThanhPhan = 0;
   
   // ---- lưu dữ liệu cho thành phần ảnh
   unsigned short soHang = 0;
   while( soHang < beCao ) {
      
      // ---- tính số lượng hàng còn
      unsigned short soLuongHangCon = beCao - soHang;
      if( soLuongHangCon > 16 )
         soLuongHangCon = 16;
      else
         beDaiDem = (beDaiDem >> 4)*soLuongHangCon;   // bề dài đệm cho thành phần dư (không đủ 16 hàng)
      
      bangThanhPhan[soThanhPhan] = ftell( tep );
      soThanhPhan++;
      
      // ---- luư số hàng
      fputc( soHang & 0xff, tep );
      fputc( (soHang >> 8), tep );
      fputc( (soHang >> 16), tep );
      fputc( (soHang >> 24), tep );
      
      // ---- dữ liệu kênh
      unsigned int diaChiKenhBatDau = beRong*(beCao - soHang - 1);
      unsigned int diaChiDem = 0;
      
      // ---- gồm dữ liệu từ các hàng
      unsigned char soHangTrongThanhPhan = 0;
      
      while( soHangTrongThanhPhan < soLuongHangCon ) {
         
         // ---- tùy kiểu dữ liệu trong ảnh
         if( kieuDuLieu == kKIEU_FLOAT ) {
            // ---- chép kênh xanh
            chepDuLieuKenhFloat( &(dem[diaChiDem]), &(anh->kenhXanh[diaChiKenhBatDau]), beRong );
            // ---- chép kênh lục
            diaChiDem += beRong << kieuDuLieu;
            chepDuLieuKenhFloat( &(dem[diaChiDem]), &(anh->kenhLuc[diaChiKenhBatDau]), beRong );
            // ---- chép kênh đỏ
            diaChiDem += beRong << kieuDuLieu;
            chepDuLieuKenhFloat( &(dem[diaChiDem]), &(anh->kenhDo[diaChiKenhBatDau]), beRong );
            // ---- tiếp theo
            diaChiDem += beRong << kieuDuLieu;
            diaChiKenhBatDau -= beRong;
         }
         else {  // kKIEU_HALF
            // ---- chép kênh xanh
            chepDuLieuKenhHalf( &(dem[diaChiDem]), &(anh->kenhXanh[diaChiKenhBatDau]), beRong );
            // ---- chép kênh lục
            diaChiDem += beRong << kieuDuLieu;
            chepDuLieuKenhHalf( &(dem[diaChiDem]), &(anh->kenhLuc[diaChiKenhBatDau]), beRong );
            // ---- chép kênh đỏ
            diaChiDem += beRong << kieuDuLieu;
            chepDuLieuKenhHalf( &(dem[diaChiDem]), &(anh->kenhDo[diaChiKenhBatDau]), beRong );
            // ---- tiếp theo
            diaChiDem += beRong << kieuDuLieu;
            diaChiKenhBatDau -= beRong;
         }
         
         soHangTrongThanhPhan++;
      }
      
      locDuLieuTrongDem( dem, beDaiDem, demLoc);
      unsigned int beDaiDuLieuNen = nenZIP( demLoc, beDaiDem, demNenZIP, beDaiDem << 1 );
      
      fputc( beDaiDuLieuNen & 0xff, tep );
      fputc( (beDaiDuLieuNen >> 8), tep );
      fputc( (beDaiDuLieuNen >> 16), tep );
      fputc( (beDaiDuLieuNen >> 24), tep );
      
      // ---- lưu dữ liệu nén
      unsigned int diaChi = 0;
      while( diaChi < beDaiDuLieuNen ) {
         fputc( demNenZIP[diaChi], tep );
         diaChi++;
      }
      
      soHang += 16;
   }
   
   // ---- lưu bảng thành phân
   fseek( tep, diaChiDauBangThanhPhan, SEEK_SET );
   soHang = 0;
   
   soThanhPhan = 0;
   while( soThanhPhan < soLuongThanhPhan ) {
      unsigned long long diaChiThanhPhan = bangThanhPhan[soThanhPhan];
      fputc( diaChiThanhPhan & 0xff, tep );
      fputc( (diaChiThanhPhan >> 8), tep );
      fputc( (diaChiThanhPhan >> 16), tep );
      fputc( (diaChiThanhPhan >> 24), tep );
      fputc( (diaChiThanhPhan >> 32), tep );
      fputc( (diaChiThanhPhan >> 40), tep );
      fputc( (diaChiThanhPhan >> 48), tep );
      fputc( (diaChiThanhPhan >> 56), tep );
      soThanhPhan++;
   }
   
   // ---- thả trí nhớ
   free( dem );
   free( demLoc );
   free( demNenZIP );
   // ---- đóng tệp
   fclose( tep );
}

void chepDuLieuKenhFloat( unsigned char *dem, const float *kenh, unsigned short beRong ) {
   
   unsigned short soCot = 0;
   unsigned int diaChiDem = 0;
   while( soCot < beRong ) {
      
      union {
         unsigned int i;
         float f;
      } u_if;
      
      u_if.f = kenh[soCot];
      dem[diaChiDem] = u_if.i & 0xff;
      dem[diaChiDem + 1] = (u_if.i >> 8) & 0xff;
      dem[diaChiDem + 2] = (u_if.i >> 16) & 0xff;
      dem[diaChiDem + 3] = (u_if.i >> 24) & 0xff;
      diaChiDem += 4;
      soCot++;
   }
   
}

void chepDuLieuKenhHalf( unsigned char *dem, const float *kenh, unsigned short beRong ) {
   
   unsigned short soCot = 0;
   unsigned int diaChiDem = 0;
   while( soCot < beRong ) {
      
      unsigned short h = doiFloatSangHalf( kenh[soCot] );
      dem[diaChiDem] = h & 0xff;
      dem[diaChiDem + 1] = (h >> 8) & 0xff;
      diaChiDem += 2;
      soCot++;
   }
   
}

void luuThongTinKenh_EXR( FILE *tep, unsigned char *danhSachKenh, unsigned char soLuongKenh, unsigned char kieuDuLieu ) {
   
   fprintf( tep, "channels" );
   fputc( 0x00, tep );
   fprintf( tep, "chlist" );
   fputc( 0x00, tep );
   unsigned char beDaiDuLieuKenh = soLuongKenh*18 + 1;
   fputc( beDaiDuLieuKenh, tep );   // bề dài cho n kênh, tên các kênh dài một chữ cái ASCII
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   // ---- thông tin cho các kênh
   unsigned char soKenh = 0;
   while( soKenh < soLuongKenh ) {
      fputc( danhSachKenh[soKenh], tep );
      fputc( 0x00, tep );
      
      fputc( kieuDuLieu, tep );  // kiểu dữ liệu 0x02 nghỉa là float, 0x01 là half
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      
      fputc( 0x00, tep );   // chỉ xài cho phương pháp nén B44, ở đây không xài
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      
      fputc( 0x01, tep );  // nhịp x
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      
      fputc( 0x01, tep );  // nhịp y
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      
      soKenh++;
   }
   
   // ---- kết thúc danh sách kênh
   fputc( 0x00, tep );
}

void luuThongTinCuaSoDuLieu( FILE *tep, unsigned int beRong, unsigned int beCao ) {
   beRong--;  // số cột cuối
   beCao--;   // số hàng cuối
   fprintf( tep, "dataWindow" );
   fputc( 0x00, tep );
   fprintf( tep, "box2i" );
   fputc( 0x00, tep );
   fputc( 16, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   // ---- góc x
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- góc y
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- cột cuối
   fputc( beRong & 0xff, tep );
   fputc( (beRong >> 8) & 0xff, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- hàng cuối
   fputc( beCao & 0xff, tep );
   fputc( (beCao >> 8), tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
}

void luuThongTinCuaSoChieu( FILE *tep, unsigned int beRong, unsigned int beCao ) {
   beRong--;  // số cột cuối
   beCao--;   // số hàng cuối
   fprintf( tep, "displayWindow" );
   fputc( 0x00, tep );
   fprintf( tep, "box2i" );
   fputc( 0x00, tep );
   fputc( 16, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   
   // ---- góc x
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- góc y
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- cột cuối
   fputc( beRong & 0xff, tep );
   fputc( (beRong >> 8) & 0xff, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
   // ---- hàng cuối
   fputc( beCao & 0xff, tep );
   fputc( (beCao >> 8), tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );
}

void luuThoiGianKetXuat( FILE *tep, unsigned short thoiGianKetXuat ) {
   
   // ---- tỉ số cạnh điểm ảnh
   fprintf( tep, "RenderTime" );
   fputc( 0x00, tep );
   fprintf( tep, "string" );
   fputc( 0x00, tep );
   
   // ---- phút
   unsigned short phut = thoiGianKetXuat/60;
   if( phut < 100 ) {
      fputc( 0x08, tep );   // bề dài dữ liệu
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fprintf( tep, "%02d", phut );   // đổi thành thập phận
   }
   else {
      fputc( 0x0a, tep );   // bề dài dữ liệu
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fputc( 0x00, tep );
      fprintf( tep, "%04d", phut );   // đổi thành thập phận
   }
   // ---- giây
   fputc( ':', tep );
   thoiGianKetXuat -= phut*60;
   fprintf( tep, "%02d", thoiGianKetXuat );  // đổi thành thập phận
   fputc( '.', tep );
   fputc( '0', tep );
   fputc( '0', tep );
}

void luuBangDuLieuAnh( FILE *tep, unsigned short soLuongThanhPhan ) {
   
   // ---- chưa biết địa chỉ thành phần vì chưa nén dữ liệu, sau nén sẽ đặt gía trị trong bảng
   unsigned long long diaChiThanhPhan = 0x0;
   
   unsigned short soThanhPhan = 0;
   while( soThanhPhan < soLuongThanhPhan ) {
      fputc( diaChiThanhPhan & 0xff, tep );
      fputc( (diaChiThanhPhan >> 8), tep );
      fputc( (diaChiThanhPhan >> 16), tep );
      fputc( (diaChiThanhPhan >> 24), tep );
      fputc( (diaChiThanhPhan >> 32), tep );
      fputc( (diaChiThanhPhan >> 40), tep );
      fputc( (diaChiThanhPhan >> 48), tep );
      fputc( (diaChiThanhPhan >> 56), tep );
      soThanhPhan++;
   }
   
}

void luuDuLieuKenhFloat( FILE *tep, const float *kenh, unsigned int diaChi, unsigned short beRong ) {
   
   unsigned short soCot = 0;
   while( soCot < beRong ) {
      
      union {
         float f;
         unsigned int i;
      } ui;
      
      ui.f = kenh[diaChi + soCot];
      fputc( ui.i & 0xff, tep );
      fputc( (ui.i >> 8), tep );
      fputc( (ui.i >> 16), tep );
      fputc( (ui.i >> 24), tep );
      soCot++;
   }
   // ---- kênh tiếp
}

void luuDuLieuKenhHalf( FILE *tep, const float *kenh, unsigned int diaChi, unsigned short beRong ) {
   
   unsigned short soCot = 0;
   while( soCot < beRong ) {
      
      unsigned short h = doiFloatSangHalf( kenh[diaChi + soCot] );
      fputc( h & 0xff, tep );
      fputc( (h >> 8), tep );
      soCot++;
   }
   // ---- kênh tiếp
}

// ---- rất đơn giản, cho tốc đ`ộ nhanh và biết nguồn dữ liệu, KHÔNG THEO TOÀN CHUẨN EXR cho đổi float
// giá trị nào âm, NaN, ∞ đặt = 0,0
unsigned short doiFloatSangHalf( float soFloat ) {
   
   union {
      float f;
      unsigned int i;
   } ui;
   
   ui.f = soFloat;
   
   unsigned short soHalf;
   if( ui.i & 0x80000000 )   // âm
      soHalf = 0x0000;
   else if( (ui.f == 0.0f) || (ui.f == -0.0f) ) { // số không
      soHalf = 0x0000;
   }
   else {
      int muFloat = ui.i & 0x7f800000;
      
      if( muFloat == 0x7f800000 )  // NaN
         soHalf = 0x0000;
      else {
         char muKiemTra = (muFloat >> 23) - 127;
         
         if( muKiemTra < -14 ) // qúa nhỏ
            soHalf = 0x0000;
         else if( muKiemTra > 15 )
            soHalf = 0x7c00;  //+∞
         else {  // bình thường
            unsigned short muHalf = (((muFloat & 0x7f800000) >> 23) - 112) << 10;
            unsigned int dinhTriFloat = ui.i & 0x007fffff;
            unsigned short dinhTriHalf = ((dinhTriFloat + 0x00000fff + ((dinhTriFloat >> 13) & 1)) >> 13);
            soHalf = muHalf + dinhTriHalf;
         }
      }
   }
   
   return soHalf;
}

void locDuLieuTrongDem(unsigned char *dem, unsigned int beDai, unsigned char *demLoc ) {
   
   {
      unsigned char *t1 = demLoc;
      unsigned char *t2 = demLoc + (beDai + 1) / 2;
      char *dau = (char *)dem;  // đầu đệm cần lọc
      char *ketThuc = dau + beDai;
      unsigned char xong = kSAI;
      
      while( !xong )
      {
         if (dau < ketThuc)
            *(t1++) = *(dau++);
         else
            xong = kSAI;
         
         if (dau < ketThuc)
            *(t2++) = *(dau++);
         else
            xong = kDUNG;
      }
   }
   
   // trừ
   {
      unsigned char *t = (unsigned char *)demLoc + 1;
      unsigned char *ketThuc = (unsigned char *)demLoc + beDai;
      int p = t[-1];
      
      while (t < ketThuc) {
         
         int d = (int)(t[0]) - p + 128;
         p = t[0];
         t[0] = d;
         ++t;
      }
   }
   
}

// ---- Từ thư viện OpenEXR
unsigned int nenZIP(unsigned char *dem, int beDaiDem, unsigned char *demNen, int beDaiDemNen ) {
   
   int err;
   z_stream dongNen; // dòng nén
   
   dongNen.zalloc = Z_NULL;
   dongNen.zfree = Z_NULL;
   dongNen.opaque = Z_NULL;
   
   
   // ---- kiểm tra sai có lầm khởi đầu
   err = deflateInit(&dongNen, Z_DEFAULT_COMPRESSION);
   
   if( err != Z_OK )
      printf( "nenZip: Vấn đề khởi đầu nén %d (%x) dongNen.avail_in %d", err, err, dongNen.avail_in );
   
   // ---- cho dữ liệu cần nén
   dongNen.next_in = dem;
   dongNen.avail_in = beDaiDem;
   
   // ---- xem nếu đệm có thể chứa dữ liệu nén
   unsigned int beDaiDuDoan = (unsigned int)deflateBound(&dongNen, beDaiDem );
   if( beDaiDuDoan > beDaiDemNen )
      printf( "nenZIP: dự đoán beDaiDuDoan %d > đệm chứa beDaiDemNen %d", beDaiDuDoan, beDaiDemNen );
   
   // ---- cho đệm cho chứa dữ liệu nén
   dongNen.next_out  = demNen;
   dongNen.avail_out = beDaiDemNen;  // bề dải đệm chứa dữ liệu nén
   err = deflate(&dongNen, Z_FINISH);
   
   if( err != Z_STREAM_END ) {
      if( err == Z_OK) {
         printf( "nenZIP: Z_OK d_stream.avail_out %d d_stream.total_out %lu",
                dongNen.avail_out, dongNen.total_out );
      }
      else
         printf( "nenZIP: sai lầm hết dữ liệu sớm hơn ý định, deflate %d (%x) d_stream.avail_out %d d_stream.total_out %lu",
                err, err, dongNen.avail_in, dongNen.total_in );
   }
   
   err = deflateEnd( &dongNen );
   if( err != Z_OK )
      printf( "nenZIP: Sai lầm deflateEnd %d (%x) dongNen.avail_out %d", err, err, dongNen.avail_out );
   
   return dongNen.total_out;
}
