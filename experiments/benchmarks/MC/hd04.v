// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 20:45:14 2025

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
    new_n66_, new_n67_, new_n68_, new_n69_, new_n70_, new_n71_, new_n72_,
    new_n73_, new_n74_, new_n75_, new_n76_, new_n77_, new_n78_, new_n79_,
    new_n80_, new_n81_, new_n82_, new_n83_, new_n84_, new_n85_, new_n86_,
    new_n87_, new_n88_, new_n89_, new_n90_, new_n91_, new_n92_, new_n93_,
    new_n94_, new_n95_, new_n96_, new_n97_, new_n98_, new_n99_, new_n100_;
  assign new_n17_ = ~n1 & ~n9;
  assign new_n18_ = n9 ^ n1;
  assign new_n19_ = new_n18_ ^ new_n17_;
  assign new_n20_ = n10 ^ n2;
  assign new_n21_ = ~n2 & ~n10;
  assign new_n22_ = new_n21_ ^ new_n20_;
  assign new_n23_ = ~new_n19_ & new_n22_;
  assign new_n24_ = ~n9 & ~new_n22_;
  assign new_n25_ = ~new_n23_ & ~new_n24_;
  assign new_n26_ = n15 ^ n7;
  assign new_n27_ = n8 & n16;
  assign new_n28_ = new_n26_ & new_n27_;
  assign new_n29_ = n14 ^ n6;
  assign new_n30_ = new_n28_ & new_n29_;
  assign new_n31_ = n13 ^ n5;
  assign new_n32_ = n12 ^ n4;
  assign new_n33_ = new_n31_ & new_n32_;
  assign new_n34_ = new_n30_ & new_n33_;
  assign new_n35_ = ~new_n25_ & new_n34_;
  assign new_n36_ = n11 ^ n3;
  assign new_n37_ = ~n3 & ~n11;
  assign new_n38_ = new_n37_ ^ new_n36_;
  assign new_n39_ = ~new_n20_ & new_n38_;
  assign new_n40_ = ~new_n35_ & ~new_n39_;
  assign new_n41_ = ~n4 & ~n12;
  assign new_n42_ = new_n41_ ^ new_n32_;
  assign new_n43_ = ~n5 & ~n13;
  assign new_n44_ = new_n43_ ^ new_n31_;
  assign new_n45_ = new_n42_ & new_n44_;
  assign new_n46_ = n7 & n15;
  assign new_n47_ = ~n6 & ~n14;
  assign new_n48_ = new_n47_ ^ new_n29_;
  assign new_n49_ = ~new_n46_ & new_n48_;
  assign new_n50_ = ~new_n43_ & ~new_n47_;
  assign new_n51_ = ~new_n49_ & new_n50_;
  assign new_n52_ = ~new_n51_ & new_n45_;
  assign new_n53_ = ~new_n41_ & ~new_n52_;
  assign new_n54_ = ~new_n36_ & ~new_n53_;
  assign new_n55_ = ~new_n24_ & ~new_n34_;
  assign new_n56_ = ~new_n54_ & ~new_n55_;
  assign new_n57_ = new_n40_ & new_n56_;
  assign new_n58_ = ~new_n37_ & new_n53_;
  assign new_n59_ = new_n22_ & new_n38_;
  assign new_n60_ = ~new_n17_ & new_n59_;
  assign new_n61_ = ~new_n58_ & new_n60_;
  assign new_n62_ = ~new_n41_ & n3;
  assign new_n63_ = ~new_n52_ & new_n62_;
  assign new_n64_ = ~n1 & n11;
  assign new_n65_ = new_n24_ & new_n64_;
  assign new_n66_ = new_n63_ & new_n65_;
  assign new_n67_ = ~n9 & ~new_n21_;
  assign new_n68_ = ~n1 & ~new_n21_;
  assign new_n69_ = new_n68_ ^ new_n17_;
  assign new_n70_ = new_n69_ ^ new_n67_;
  assign new_n71_ = ~new_n66_ & new_n70_;
  assign new_n72_ = ~new_n61_ & new_n71_;
  assign new_n73_ = ~new_n57_ & ~new_n72_;
  assign new_n74_ = new_n63_ ^ new_n20_;
  assign new_n75_ = new_n53_ ^ new_n36_;
  assign new_n76_ = new_n74_ & new_n75_;
  assign new_n77_ = new_n22_ ^ new_n18_;
  assign new_n78_ = new_n77_ ^ new_n34_;
  assign new_n79_ = new_n76_ & new_n78_;
  assign new_n80_ = new_n20_ & new_n37_;
  assign new_n81_ = new_n80_ ^ new_n21_;
  assign new_n82_ = new_n81_ ^ new_n18_;
  assign new_n83_ = ~new_n76_ & new_n82_;
  assign new_n84_ = ~new_n79_ & ~new_n83_;
  assign new_n85_ = ~new_n34_ & new_n75_;
  assign new_n86_ = new_n63_ ^ new_n38_;
  assign new_n87_ = new_n86_ ^ new_n54_;
  assign new_n88_ = new_n87_ ^ new_n85_;
  assign new_n89_ = new_n88_ ^ new_n63_;
  assign new_n90_ = new_n89_ ^ new_n20_;
  assign new_n91_ = new_n75_ ^ new_n34_;
  assign new_n92_ = new_n46_ ^ new_n28_;
  assign new_n93_ = new_n29_ & new_n92_;
  assign new_n94_ = new_n93_ ^ new_n48_;
  assign new_n95_ = new_n44_ & new_n94_;
  assign new_n96_ = ~new_n43_ & ~new_n95_;
  assign new_n97_ = new_n96_ ^ new_n32_;
  assign new_n98_ = new_n94_ ^ new_n31_;
  assign new_n99_ = new_n92_ ^ new_n29_;
  assign new_n100_ = new_n27_ ^ new_n26_;
  assign po0 = new_n73_;
  assign po1 = new_n84_;
  assign po2 = new_n90_;
  assign po3 = new_n91_;
  assign po4 = new_n97_;
  assign po5 = ~new_n98_;
  assign po6 = new_n99_;
  assign po7 = new_n100_;
endmodule


