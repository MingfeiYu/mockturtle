// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 20:45:03 2025

module \/tmp/tmp  ( 
    n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, n15, n16,
    po0, po1, po2, po3, po4, po5, po6, po7  );
  input  n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14,
    n15, n16;
  output po0, po1, po2, po3, po4, po5, po6, po7;
  wire new_n17_, new_n18_, new_n19_, new_n20_, new_n21_, new_n22_, new_n23_,
    new_n24_, new_n25_, new_n26_, new_n27_, new_n28_, new_n29_, new_n30_,
    new_n31_, new_n32_, new_n33_, new_n34_, new_n35_, new_n36_, new_n37_,
    new_n38_, new_n39_, new_n40_, new_n41_, new_n42_, new_n43_, new_n44_,
    new_n45_, new_n46_, new_n47_, new_n48_, new_n49_, new_n50_, new_n51_,
    new_n52_, new_n53_, new_n54_, new_n55_, new_n56_, new_n57_, new_n58_,
    new_n59_, new_n60_, new_n61_, new_n62_, new_n63_, new_n64_, new_n65_,
    new_n66_, new_n67_, new_n68_, new_n69_, new_n70_, new_n71_;
  assign new_n17_ = n1 & n9;
  assign new_n18_ = n10 ^ n2;
  assign new_n19_ = new_n18_ ^ new_n17_;
  assign new_n20_ = new_n17_ & new_n18_;
  assign new_n21_ = n2 & n10;
  assign new_n22_ = n11 ^ n3;
  assign new_n23_ = new_n22_ ^ new_n21_;
  assign new_n24_ = new_n23_ ^ new_n20_;
  assign new_n25_ = new_n20_ & new_n23_;
  assign new_n26_ = new_n21_ & new_n22_;
  assign new_n27_ = new_n26_ ^ new_n25_;
  assign new_n28_ = n3 & n11;
  assign new_n29_ = n12 ^ n4;
  assign new_n30_ = new_n29_ ^ new_n28_;
  assign new_n31_ = new_n30_ ^ new_n27_;
  assign new_n32_ = new_n28_ & new_n29_;
  assign new_n33_ = new_n27_ & new_n30_;
  assign new_n34_ = new_n33_ ^ new_n32_;
  assign new_n35_ = n4 & n12;
  assign new_n36_ = n13 ^ n5;
  assign new_n37_ = new_n36_ ^ new_n35_;
  assign new_n38_ = new_n37_ ^ new_n34_;
  assign new_n39_ = new_n35_ & new_n36_;
  assign new_n40_ = new_n32_ & new_n37_;
  assign new_n41_ = new_n40_ ^ new_n39_;
  assign new_n42_ = new_n33_ & new_n37_;
  assign new_n43_ = new_n42_ ^ new_n41_;
  assign new_n44_ = n5 & n13;
  assign new_n45_ = n14 ^ n6;
  assign new_n46_ = new_n45_ ^ new_n44_;
  assign new_n47_ = new_n46_ ^ new_n43_;
  assign new_n48_ = new_n44_ & new_n45_;
  assign new_n49_ = new_n43_ & new_n46_;
  assign new_n50_ = new_n49_ ^ new_n48_;
  assign new_n51_ = n6 & n14;
  assign new_n52_ = n15 ^ n7;
  assign new_n53_ = new_n52_ ^ new_n51_;
  assign new_n54_ = new_n53_ ^ new_n50_;
  assign new_n55_ = new_n51_ & new_n52_;
  assign new_n56_ = new_n48_ & new_n53_;
  assign new_n57_ = new_n56_ ^ new_n55_;
  assign new_n58_ = new_n49_ & new_n53_;
  assign new_n59_ = new_n58_ ^ new_n57_;
  assign new_n60_ = n7 & n15;
  assign new_n61_ = n16 ^ n8;
  assign new_n62_ = new_n61_ ^ new_n60_;
  assign new_n63_ = new_n62_ ^ new_n59_;
  assign new_n64_ = new_n58_ & new_n62_;
  assign new_n65_ = new_n60_ & new_n61_;
  assign new_n66_ = new_n57_ & new_n62_;
  assign new_n67_ = new_n66_ ^ new_n65_;
  assign new_n68_ = new_n67_ ^ new_n64_;
  assign new_n69_ = n8 & n16;
  assign new_n70_ = new_n69_ ^ new_n61_;
  assign new_n71_ = new_n70_ ^ new_n68_;
  assign po0 = new_n19_;
  assign po1 = new_n24_;
  assign po2 = new_n31_;
  assign po3 = new_n38_;
  assign po4 = new_n47_;
  assign po5 = new_n54_;
  assign po6 = new_n63_;
  assign po7 = new_n71_;
endmodule


