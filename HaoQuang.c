//  HàoQuang.c
//
//  Kết xuất ảnh hào quang phóng để ghép ảnh với phần mềm Blender
//      (chuẩn ảnh 4k: 4096 x 2160 điểm ảnh)
//  Phiên Bản 0.6
//  Phát hành 2561.02.10
//  Khởi đầu 2561.02.09

//  Biên dịch cho gcc: gcc -lm -lz HaoQuang.c -o <tên chương trình>
//  Biên dịch cho clang: clang -lm -lz HaoQuang.c -o <tên chương trình>

//  Dùng <tên chương trình> <vị trí x> <vị trí y> <số hoạt hình đầu> <số hoạt hình cuối> <tốc độ nở> <tỉ số phóng to>

//
//

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
   float dd;
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
   float tamX;              // tâm X
   float tamY;              // tâm Y
   Mau mau;                 // màu chân trời
   float banKinhTrong;      // bán kính trong, điểm ảnh
   float banKinhGiuaTrong;  // bán kính giữa trong, điểm ảnh
   float banKinhGiuaNgoai;  // bán kính giữa ngoài, điểm ảnh
   float banKinhNgoai;      // bán kính ngoài, điểm ảnh
   
   float tocDoNo;           // tốc độ nở
} HaoQuang;


/* Hào Quang */
HaoQuang hoaQuangSieuKhongGian( Mau *mau, unsigned  short x, unsigned y, float tocDo, float tiSo );

/* Tính Hào Quang */
void tinhHaoQuang( Anh *anhBauTroi, HaoQuang *haoQuang, unsigned short soHoatHinh, float phaiMau );

void docThamSoHoatHinh( int argc, char **argv, char *xauKemTen, unsigned int *viTriX, unsigned int *viTriY,
                       unsigned int *soHoatHinhDau, unsigned int *soHoatHinhCuoi,
                       float *tocDo, float *tiSo );

// ---- lưu ảnh RLE
void luuAnhZIP( char *tenTep, Anh *anh, unsigned char kieuDuLieu, unsigned short thoiGianKetXuat );   // lưu ảnh ZIP

/* Ảnh */
Anh taoAnhVoiCoKich( unsigned short beRong, unsigned short beCao, float coKichDiemAnh );
void xoaAnh( Anh *anh );


int main( int argc, char **argv ) {

   float tiSo = 0.25f;
   unsigned short beRong = 4096*tiSo;
   unsigned short beCao = 2160*tiSo;
   Anh anhHaoQuang = taoAnhVoiCoKich( beRong, beCao, 1.0f );

   if( argc > 6 ) {
      char xauKemTen[255];
      unsigned int viTriTamX = 0;
      unsigned int viTriTamY = 0;
      unsigned int soHoatHinhDau = 0;
      unsigned int soHoatHinhCuoi = 0;
      float tocDo;
      float tiSoPhongTo;

      docThamSoHoatHinh( argc, argv, xauKemTen, &viTriTamX, &viTriTamY, &soHoatHinhDau, &soHoatHinhCuoi,
                              &tocDo, &tiSoPhongTo );
      
      float buocPhaiMau = 1.0f/(soHoatHinhCuoi - soHoatHinhDau);
      float phaiMau = 1.0f;
      printf( "buocPhaiMau %5.3f\n", buocPhaiMau );
      
      Mau mauHaoQuang;
      mauHaoQuang.d = 1.0f;
      mauHaoQuang.l = 1.0f;
      mauHaoQuang.x = 1.0f;
      mauHaoQuang.dd = 1.0f;

      HaoQuang haoQuang = hoaQuangSieuKhongGian( &mauHaoQuang, viTriTamX, viTriTamY, tocDo, tiSo );
      while ( soHoatHinhDau < soHoatHinhCuoi ) {
         
         char tenAnh[256];
         sprintf( tenAnh, "HaoQuang_%s_%03d.exr", xauKemTen, soHoatHinhDau );
         
         // ---- bầu trời
         tinhHaoQuang( &anhHaoQuang, &haoQuang, soHoatHinhDau, phaiMau );

         luuAnhZIP( tenAnh, &anhHaoQuang, kKIEU_FLOAT, 0 );
         printf( "Lưu ảnh %s:\n", tenAnh );
         soHoatHinhDau++;
         phaiMau -= buocPhaiMau;
      }
      xoaAnh( &anhHaoQuang );
      
      printf( "%d %d điểm ảnh\n", beRong, beCao );
   }
   else {
      printf( "Cach dùng <tên chương trình> <xâu kèm tên> <vị trí x> <vị trí y> <số hoạt hình đầu> <số hoạt hình cuối> <tốc độ nở> <tỉ số phóng to>\n" );
   }
   return 1;
}


#pragma mark ---- Hào Quang
HaoQuang hoaQuangSieuKhongGian( Mau *mau, unsigned  short x, unsigned y, float tocDo, float tiSo ) {
   
   HaoQuang haoQuangSieuKhongGian;
   
   haoQuangSieuKhongGian.tamX = x*tiSo;                  // vị trí trung tâm hào quang x
   haoQuangSieuKhongGian.tamY = y*tiSo;                  // vị trí trung tâm hào quang y
   haoQuangSieuKhongGian.mau = *mau;                // màu hào quang
   
   haoQuangSieuKhongGian.banKinhTrong = -200.0f*tiSo;      // bán kính trong, điểm ảnh
   haoQuangSieuKhongGian.banKinhGiuaTrong = 20.0f*tiSo;  // bán kính giữa trong, điểm ảnh
   haoQuangSieuKhongGian.banKinhGiuaNgoai = 22.0f*tiSo;  // bán kính giữa ngoài, điểm ảnh
   haoQuangSieuKhongGian.banKinhNgoai = 50.0f*tiSo;      // bán kính ngoài, điểm ảnh
   
   haoQuangSieuKhongGian.tocDoNo = tocDo*tiSo;            // tốc độ nở ra
   
   return haoQuangSieuKhongGian;
}


/* Hàm để tính ảnh bầu trời */
// +----------------------+
// |       -------        |
// |    /    ---    \     |
// |  |   /       \   |   |
// |  |  |    +    |  |   |
// |  |   \       /   |   |
// |    \    ---    /     |
// |       -------        |
// +----------------------+
// 

void tinhHaoQuang( Anh *anhHaoQuang, HaoQuang *haoQuang, unsigned short soHoatHinh, float phaiMau  ) {

   float cachNo = haoQuang->tocDoNo*soHoatHinh;

   float cachNgoai = haoQuang->banKinhNgoai - haoQuang->banKinhGiuaNgoai;
   float cachGiua = haoQuang->banKinhGiuaNgoai - haoQuang->banKinhGiuaTrong;
   float cachTrong = haoQuang->banKinhGiuaTrong - haoQuang->banKinhTrong + cachNo*0.5f;

   // ---- quét ngang
   unsigned int diaChiTrongKenh = 0;
   
   unsigned short soHang = 0;
   while( soHang < anhHaoQuang->beCao ) {
      unsigned short soCot = 0;
      while( soCot < anhHaoQuang->beRong ) {
         
         // ---- tính bán kính
         float cachX = soCot - haoQuang->tamX;
         float cachY = soHang - haoQuang->tamY;
   
         float cachDiemAnh = sqrtf( cachX*cachX + cachY*cachY );
         
         if( cachDiemAnh > (haoQuang->banKinhNgoai + cachNo) ) {
            anhHaoQuang->kenhDo[diaChiTrongKenh] = 0.0f;
            anhHaoQuang->kenhLuc[diaChiTrongKenh] = 0.0f;
            anhHaoQuang->kenhXanh[diaChiTrongKenh] = 0.0f;
            anhHaoQuang->kenhDuc[diaChiTrongKenh] = 0.0f;
         }
         else if( cachDiemAnh > (haoQuang->banKinhGiuaNgoai + cachNo) ) {
            float soPhan = 1.0f - (cachDiemAnh - haoQuang->banKinhGiuaNgoai - cachNo)/cachNgoai;

            anhHaoQuang->kenhDo[diaChiTrongKenh] = haoQuang->mau.d*soPhan*phaiMau;
            anhHaoQuang->kenhLuc[diaChiTrongKenh] = haoQuang->mau.d*soPhan*phaiMau;
            anhHaoQuang->kenhXanh[diaChiTrongKenh] = haoQuang->mau.d*soPhan*phaiMau;
            anhHaoQuang->kenhDuc[diaChiTrongKenh] = soPhan*phaiMau;
         }
         else if( cachDiemAnh > (haoQuang->banKinhGiuaTrong + cachNo) ) {
            anhHaoQuang->kenhDo[diaChiTrongKenh] = haoQuang->mau.d*phaiMau;
            anhHaoQuang->kenhLuc[diaChiTrongKenh] = haoQuang->mau.l*phaiMau;
            anhHaoQuang->kenhXanh[diaChiTrongKenh] = haoQuang->mau.x*phaiMau;
            anhHaoQuang->kenhDuc[diaChiTrongKenh] = haoQuang->mau.dd*phaiMau;
         }
         else if( cachDiemAnh > (haoQuang->banKinhTrong + cachNo*0.5) ) {
            float soPhan = (cachDiemAnh - haoQuang->banKinhTrong - cachNo*0.5f)/cachTrong;
            
            anhHaoQuang->kenhDo[diaChiTrongKenh] = haoQuang->mau.d*soPhan*phaiMau;
            anhHaoQuang->kenhLuc[diaChiTrongKenh] = haoQuang->mau.d*soPhan*phaiMau;
            anhHaoQuang->kenhXanh[diaChiTrongKenh] = haoQuang->mau.d*soPhan*phaiMau;
            anhHaoQuang->kenhDuc[diaChiTrongKenh] = soPhan*phaiMau;
         }
         else {
            anhHaoQuang->kenhDo[diaChiTrongKenh] = 0.0f;
            anhHaoQuang->kenhLuc[diaChiTrongKenh] = 0.0f;
            anhHaoQuang->kenhXanh[diaChiTrongKenh] = 0.0f;
            anhHaoQuang->kenhDuc[diaChiTrongKenh] = 0.0f;
         }
         
         diaChiTrongKenh++;
         soCot++;
      }
      
      soHang++;
   }
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

#pragma mark ----
void docThamSoHoatHinh( int argc, char **argv, char *xauKemTen,  unsigned int *viTriX, unsigned int *viTriY,
                       unsigned int *soHoatHinhDau, unsigned int *soHoatHinhCuoi,
                      float *tocDo, float *tiSo ) {
   
   if( argc > 7 ) {
      sscanf( argv[1], "%s", xauKemTen );
      sscanf( argv[2], "%u", viTriX );
      sscanf( argv[3], "%u", viTriY );
      sscanf( argv[4], "%u", soHoatHinhDau );
      sscanf( argv[5], "%u", soHoatHinhCuoi );
      sscanf( argv[6], "%f", tocDo );
      sscanf( argv[7], "%f", tiSo );

      // ---- kiểm tra soHoatHinhDau nhỏ soHoatHinhCuoi
      if( *soHoatHinhDau >= *soHoatHinhCuoi )
         *soHoatHinhCuoi = *soHoatHinhDau + 1;
   }
   else {
      *soHoatHinhDau = 0;
   }
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
   unsigned char danhSachKenh[4] = {'A', 'B', 'G', 'R'};
   luuThongTinKenh_EXR( tep, danhSachKenh, 4, kieuDuLieu );
   
   // ---- chu dẫn
   fprintf( tep, "comments" );
   fputc( 0x00, tep );
   fprintf( tep, "string" );
   fputc( 0x00, tep );
   char *chuDan = "CC0 - Hồ Nhựt Châu\x00";
   // ---- tìm bề dài xâu
   unsigned short chiSoXau = 0;
   while( chuDan[chiSoXau] ) {
      chiSoXau++;
   }

   fputc( chiSoXau & 0xff, tep );   // bề dài dữ liệu
   fputc( chiSoXau >> 8, tep );
   fputc( 0x00, tep );
   fputc( 0x00, tep );

   fprintf( tep, "%s", chuDan );

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
   unsigned int beDaiDem = (beRong << kieuDuLieu)*4 << 4; // nhân 4 cho 4 kênh, 16 hàng
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
            // ---- chép kênh đục
            chepDuLieuKenhFloat( &(dem[diaChiDem]), &(anh->kenhDuc[diaChiKenhBatDau]), beRong );
            // ---- chép kênh xanh
            diaChiDem += beRong << kieuDuLieu;
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
            // ---- chép kênh đục
            chepDuLieuKenhHalf( &(dem[diaChiDem]), &(anh->kenhDuc[diaChiKenhBatDau]), beRong );
            // ---- chép kênh xanh
            diaChiDem += beRong << kieuDuLieu;
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
