// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 21:05:43 2025

module \/tmp/tmp  ( 
    n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, n15, n16,
    n17, n18, n19, n20, n21, n22, n23, n24, n25, n26, n27, n28, n29, n30,
    n31, n32,
    po0, po1, po2, po3, po4, po5, po6, po7, po8, po9, po10, po11, po12,
    po13, po14, po15, po16, po17, po18, po19, po20, po21, po22, po23, po24,
    po25, po26, po27, po28, po29, po30, po31  );
  input  n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14,
    n15, n16, n17, n18, n19, n20, n21, n22, n23, n24, n25, n26, n27, n28,
    n29, n30, n31, n32;
  output po0, po1, po2, po3, po4, po5, po6, po7, po8, po9, po10, po11, po12,
    po13, po14, po15, po16, po17, po18, po19, po20, po21, po22, po23, po24,
    po25, po26, po27, po28, po29, po30, po31;
  wire new_n33_, new_n34_, new_n35_, new_n36_, new_n37_, new_n38_, new_n39_,
    new_n40_, new_n41_, new_n42_, new_n43_, new_n44_, new_n45_, new_n46_,
    new_n47_, new_n48_, new_n49_, new_n50_, new_n51_, new_n52_, new_n53_,
    new_n54_, new_n55_, new_n56_, new_n57_, new_n58_, new_n59_, new_n60_,
    new_n61_, new_n62_, new_n63_, new_n64_, new_n65_, new_n66_, new_n67_,
    new_n68_;
  assign new_n33_ = ~n13 & ~n14;
  assign new_n34_ = ~n15 & ~n16;
  assign new_n35_ = new_n33_ & new_n34_;
  assign new_n36_ = ~n9 & ~n10;
  assign new_n37_ = ~n11 & ~n12;
  assign new_n38_ = new_n36_ & new_n37_;
  assign new_n39_ = new_n35_ & new_n38_;
  assign new_n40_ = ~n5 & ~n6;
  assign new_n41_ = ~n7 & ~n8;
  assign new_n42_ = new_n40_ & new_n41_;
  assign new_n43_ = ~n1 & ~n2;
  assign new_n44_ = ~n3 & ~n4;
  assign new_n45_ = new_n43_ & new_n44_;
  assign new_n46_ = new_n42_ & new_n45_;
  assign new_n47_ = ~new_n39_ & ~new_n46_;
  assign new_n48_ = ~n29 & ~n30;
  assign new_n49_ = ~n31 & ~n32;
  assign new_n50_ = new_n48_ & new_n49_;
  assign new_n51_ = ~n25 & ~n26;
  assign new_n52_ = ~n27 & ~n28;
  assign new_n53_ = new_n51_ & new_n52_;
  assign new_n54_ = new_n50_ & new_n53_;
  assign new_n55_ = ~n21 & ~n22;
  assign new_n56_ = ~n23 & ~n24;
  assign new_n57_ = new_n55_ & new_n56_;
  assign new_n58_ = ~n17 & ~n18;
  assign new_n59_ = ~n19 & ~n20;
  assign new_n60_ = new_n58_ & new_n59_;
  assign new_n61_ = new_n57_ & new_n60_;
  assign new_n62_ = ~new_n61_ & new_n54_;
  assign new_n63_ = new_n62_ ^ new_n61_;
  assign new_n64_ = new_n47_ & new_n63_;
  assign new_n65_ = new_n64_ ^ new_n47_;
  assign new_n66_ = ~new_n39_ & new_n62_;
  assign new_n67_ = new_n66_ ^ new_n39_;
  assign new_n68_ = ~new_n46_ & new_n67_;
  assign po29 = new_n65_;
  assign po30 = new_n64_;
  assign po31 = new_n68_;
  assign po0 = 1'b0;
  assign po1 = 1'b0;
  assign po2 = 1'b0;
  assign po3 = 1'b0;
  assign po4 = 1'b0;
  assign po5 = 1'b0;
  assign po6 = 1'b0;
  assign po7 = 1'b0;
  assign po8 = 1'b0;
  assign po9 = 1'b0;
  assign po10 = 1'b0;
  assign po11 = 1'b0;
  assign po12 = 1'b0;
  assign po13 = 1'b0;
  assign po14 = 1'b0;
  assign po15 = 1'b0;
  assign po16 = 1'b0;
  assign po17 = 1'b0;
  assign po18 = 1'b0;
  assign po19 = 1'b0;
  assign po20 = 1'b0;
  assign po21 = 1'b0;
  assign po22 = 1'b0;
  assign po23 = 1'b0;
  assign po24 = 1'b0;
  assign po25 = 1'b0;
  assign po26 = 1'b0;
  assign po27 = 1'b0;
  assign po28 = 1'b0;
endmodule


