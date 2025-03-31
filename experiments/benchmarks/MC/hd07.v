// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 20:45:51 2025

module \/tmp/tmp  ( 
    n1, n2, n3, n4, n5, n6, n7, n8,
    po0, po1, po2, po3, po4, po5, po6, po7  );
  input  n1, n2, n3, n4, n5, n6, n7, n8;
  output po0, po1, po2, po3, po4, po5, po6, po7;
  wire new_n9_, new_n10_, new_n11_, new_n12_, new_n13_, new_n14_, new_n15_,
    new_n16_, new_n17_, new_n18_, new_n19_, new_n20_, new_n21_, new_n22_,
    new_n23_, new_n24_, new_n25_;
  assign new_n9_ = ~n2 & n1;
  assign new_n10_ = n3 & new_n9_;
  assign new_n11_ = ~n3 & n2;
  assign new_n12_ = new_n11_ ^ new_n9_;
  assign new_n13_ = n4 & new_n12_;
  assign new_n14_ = ~n3 & ~n4;
  assign new_n15_ = n4 & n5;
  assign new_n16_ = new_n15_ ^ new_n14_;
  assign new_n17_ = ~new_n12_ & new_n16_;
  assign new_n18_ = ~new_n17_ & n5;
  assign new_n19_ = ~new_n17_ & n6;
  assign new_n20_ = ~n6 & n5;
  assign new_n21_ = ~n7 & n6;
  assign new_n22_ = new_n21_ ^ new_n20_;
  assign new_n23_ = ~new_n22_ & new_n17_;
  assign new_n24_ = ~new_n23_ & n7;
  assign new_n25_ = ~new_n23_ & n8;
  assign po2 = new_n10_;
  assign po3 = new_n13_;
  assign po4 = new_n18_;
  assign po5 = new_n19_;
  assign po6 = new_n24_;
  assign po7 = new_n25_;
  assign po0 = 1'b0;
  assign po1 = 1'b0;
endmodule


