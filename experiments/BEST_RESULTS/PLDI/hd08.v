// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 20:45:53 2025

module \/tmp/tmp  ( 
    n1, n2, n3, n4, n5, n6, n7, n8,
    po0  );
  input  n1, n2, n3, n4, n5, n6, n7, n8;
  output po0;
  wire new_n9_, new_n10_, new_n11_, new_n12_, new_n13_, new_n14_, new_n15_,
    new_n16_, new_n17_, new_n18_, new_n19_, new_n20_, new_n21_, new_n22_,
    new_n23_, new_n24_, new_n25_, new_n26_, new_n27_, new_n28_, new_n29_,
    new_n30_, new_n31_, new_n32_, new_n33_;
  assign new_n9_ = n1 & n2;
  assign new_n10_ = n2 ^ n1;
  assign new_n11_ = new_n10_ ^ new_n9_;
  assign new_n12_ = n3 & n4;
  assign new_n13_ = n4 ^ n3;
  assign new_n14_ = new_n13_ ^ new_n12_;
  assign new_n15_ = new_n11_ & new_n14_;
  assign new_n16_ = new_n14_ ^ new_n11_;
  assign new_n17_ = new_n16_ ^ new_n15_;
  assign new_n18_ = n5 & n6;
  assign new_n19_ = n6 ^ n5;
  assign new_n20_ = new_n19_ ^ new_n18_;
  assign new_n21_ = new_n17_ & new_n20_;
  assign new_n22_ = new_n20_ ^ new_n17_;
  assign new_n23_ = new_n22_ ^ new_n21_;
  assign new_n24_ = n7 & n8;
  assign new_n25_ = n8 ^ n7;
  assign new_n26_ = new_n25_ ^ new_n24_;
  assign new_n27_ = new_n26_ ^ new_n23_;
  assign new_n28_ = ~new_n18_ & ~new_n24_;
  assign new_n29_ = ~new_n9_ & ~new_n12_;
  assign new_n30_ = new_n28_ & new_n29_;
  assign new_n31_ = ~new_n15_ & new_n30_;
  assign new_n32_ = ~new_n21_ & new_n31_;
  assign new_n33_ = new_n27_ & new_n32_;
  assign po0 = new_n33_;
endmodule


