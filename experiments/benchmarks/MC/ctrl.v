// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 21:08:24 2025

module \/tmp/tmp  ( 
    n1, n2, n3, n4, n5, n6, n7,
    po0, po1, po2, po3, po4, po5, po6, po7, po8, po9, po10, po11, po12,
    po13, po14, po15, po16, po17, po18, po19, po20, po21, po22, po23, po24,
    po25  );
  input  n1, n2, n3, n4, n5, n6, n7;
  output po0, po1, po2, po3, po4, po5, po6, po7, po8, po9, po10, po11, po12,
    po13, po14, po15, po16, po17, po18, po19, po20, po21, po22, po23, po24,
    po25;
  wire new_n8_, new_n9_, new_n10_, new_n11_, new_n12_, new_n13_, new_n14_,
    new_n15_, new_n16_, new_n17_, new_n18_, new_n19_, new_n20_, new_n21_,
    new_n22_, new_n23_, new_n24_, new_n25_, new_n26_, new_n27_, new_n28_,
    new_n29_, new_n30_, new_n31_, new_n32_, new_n33_, new_n34_, new_n35_,
    new_n36_, new_n37_, new_n38_, new_n39_, new_n40_, new_n41_, new_n42_,
    new_n43_, new_n44_, new_n45_, new_n46_, new_n47_, new_n48_, new_n49_,
    new_n50_, new_n51_, new_n52_, new_n53_, new_n54_, new_n55_, new_n56_,
    new_n57_, new_n58_, new_n59_, new_n60_, new_n61_, new_n62_, new_n63_,
    new_n64_, new_n65_, new_n66_, new_n67_, new_n68_, new_n69_, new_n70_,
    new_n71_, new_n72_, new_n73_, new_n74_, new_n75_, new_n76_, new_n77_,
    new_n78_, new_n79_, new_n80_, new_n81_, new_n82_, new_n83_, new_n84_,
    new_n85_, new_n86_, new_n87_, new_n88_, new_n89_, new_n90_, new_n91_,
    new_n92_, new_n93_, new_n94_, new_n95_, new_n96_, new_n97_, new_n98_,
    new_n99_, new_n100_, new_n101_, new_n102_, new_n103_, new_n104_,
    new_n105_, new_n106_, new_n107_, new_n108_, new_n109_, new_n110_,
    new_n111_, new_n112_;
  assign new_n8_ = ~n5 & n4;
  assign new_n9_ = n3 & new_n8_;
  assign new_n10_ = ~n4 & n2;
  assign new_n11_ = new_n8_ ^ n4;
  assign new_n12_ = new_n11_ ^ new_n10_;
  assign new_n13_ = new_n11_ ^ n5;
  assign new_n14_ = ~n2 & new_n13_;
  assign new_n15_ = new_n14_ ^ new_n12_;
  assign new_n16_ = n3 & new_n15_;
  assign new_n17_ = new_n16_ ^ new_n9_;
  assign new_n18_ = new_n13_ ^ n4;
  assign new_n19_ = ~new_n18_ & n3;
  assign new_n20_ = new_n19_ ^ new_n17_;
  assign new_n21_ = ~n2 & ~n3;
  assign new_n22_ = new_n21_ ^ new_n20_;
  assign new_n23_ = new_n21_ ^ n3;
  assign new_n24_ = n1 & new_n11_;
  assign new_n25_ = new_n24_ ^ new_n11_;
  assign new_n26_ = new_n21_ & new_n25_;
  assign new_n27_ = new_n26_ ^ new_n23_;
  assign new_n28_ = new_n27_ ^ new_n11_;
  assign new_n29_ = new_n28_ ^ new_n22_;
  assign new_n30_ = ~new_n13_ & ~new_n22_;
  assign new_n31_ = new_n30_ ^ new_n27_;
  assign new_n32_ = ~n2 & new_n8_;
  assign new_n33_ = new_n32_ ^ new_n8_;
  assign new_n34_ = new_n13_ ^ n2;
  assign new_n35_ = ~n1 & n2;
  assign new_n36_ = ~new_n35_ & new_n13_;
  assign new_n37_ = new_n36_ ^ new_n34_;
  assign new_n38_ = new_n37_ ^ new_n33_;
  assign new_n39_ = ~new_n27_ & ~new_n38_;
  assign new_n40_ = ~new_n21_ & n4;
  assign new_n41_ = new_n40_ ^ new_n18_;
  assign new_n42_ = ~new_n24_ & new_n41_;
  assign new_n43_ = n3 ^ n1;
  assign new_n44_ = ~n3 & n1;
  assign new_n45_ = new_n44_ ^ new_n43_;
  assign new_n46_ = new_n13_ ^ n3;
  assign new_n47_ = ~n3 & new_n13_;
  assign new_n48_ = new_n47_ ^ new_n46_;
  assign new_n49_ = ~new_n45_ & ~new_n48_;
  assign new_n50_ = n6 & new_n11_;
  assign new_n51_ = new_n35_ & new_n50_;
  assign new_n52_ = ~n7 & n2;
  assign new_n53_ = new_n50_ & new_n52_;
  assign new_n54_ = new_n53_ ^ new_n32_;
  assign new_n55_ = new_n44_ & new_n54_;
  assign new_n56_ = new_n35_ ^ n2;
  assign new_n57_ = new_n50_ ^ new_n8_;
  assign new_n58_ = new_n56_ & new_n57_;
  assign new_n59_ = n1 & new_n32_;
  assign new_n60_ = ~n3 & ~new_n59_;
  assign new_n61_ = ~new_n58_ & new_n60_;
  assign new_n62_ = new_n61_ ^ new_n55_;
  assign new_n63_ = ~new_n51_ & new_n62_;
  assign new_n64_ = ~new_n63_ & new_n49_;
  assign new_n65_ = ~n7 & n5;
  assign new_n66_ = ~new_n65_ & n4;
  assign new_n67_ = ~n3 & ~new_n66_;
  assign new_n68_ = ~new_n48_ & n2;
  assign new_n69_ = ~new_n67_ & new_n68_;
  assign new_n70_ = ~new_n56_ & new_n11_;
  assign new_n71_ = new_n70_ ^ n3;
  assign new_n72_ = ~new_n70_ & n3;
  assign new_n73_ = new_n72_ ^ new_n71_;
  assign new_n74_ = new_n9_ ^ n3;
  assign new_n75_ = new_n74_ ^ new_n48_;
  assign new_n76_ = new_n75_ ^ new_n18_;
  assign new_n77_ = ~new_n73_ & new_n76_;
  assign new_n78_ = new_n15_ ^ n5;
  assign new_n79_ = new_n78_ ^ new_n24_;
  assign new_n80_ = n3 & new_n79_;
  assign new_n81_ = ~new_n26_ & ~new_n80_;
  assign new_n82_ = ~new_n75_ & n2;
  assign new_n83_ = n2 ^ n1;
  assign new_n84_ = ~new_n83_ & new_n47_;
  assign new_n85_ = new_n84_ ^ n3;
  assign new_n86_ = new_n14_ & new_n44_;
  assign new_n87_ = new_n86_ ^ new_n85_;
  assign new_n88_ = new_n47_ ^ new_n21_;
  assign new_n89_ = new_n88_ ^ new_n87_;
  assign new_n90_ = new_n82_ & new_n89_;
  assign new_n91_ = new_n10_ ^ n2;
  assign new_n92_ = new_n91_ ^ new_n32_;
  assign new_n93_ = ~new_n87_ & ~new_n92_;
  assign new_n94_ = ~new_n48_ & ~new_n93_;
  assign new_n95_ = new_n94_ ^ new_n16_;
  assign new_n96_ = new_n95_ ^ new_n76_;
  assign new_n97_ = ~n1 & ~new_n18_;
  assign new_n98_ = new_n21_ & new_n97_;
  assign new_n99_ = new_n35_ ^ n1;
  assign new_n100_ = n5 & new_n99_;
  assign new_n101_ = ~n4 & ~new_n100_;
  assign new_n102_ = ~n3 & ~new_n101_;
  assign new_n103_ = ~new_n16_ & ~new_n102_;
  assign new_n104_ = ~new_n18_ & new_n45_;
  assign new_n105_ = new_n104_ ^ new_n19_;
  assign new_n106_ = new_n32_ & new_n45_;
  assign new_n107_ = new_n59_ ^ n3;
  assign new_n108_ = new_n107_ ^ new_n60_;
  assign new_n109_ = new_n9_ & new_n56_;
  assign new_n110_ = new_n9_ & new_n35_;
  assign new_n111_ = new_n72_ ^ new_n61_;
  assign new_n112_ = new_n72_ ^ new_n62_;
  assign po0 = new_n29_;
  assign po1 = ~new_n31_;
  assign po2 = new_n39_;
  assign po3 = new_n42_;
  assign po4 = new_n64_;
  assign po5 = new_n69_;
  assign po6 = new_n77_;
  assign po7 = ~new_n81_;
  assign po8 = new_n90_;
  assign po9 = new_n96_;
  assign po10 = new_n94_;
  assign po11 = new_n98_;
  assign po12 = ~new_n103_;
  assign po13 = new_n105_;
  assign po14 = new_n104_;
  assign po15 = new_n106_;
  assign po16 = ~new_n108_;
  assign po17 = new_n109_;
  assign po18 = new_n110_;
  assign po19 = new_n19_;
  assign po20 = ~new_n111_;
  assign po21 = new_n55_;
  assign po22 = ~new_n112_;
  assign po24 = new_n84_;
  assign po25 = new_n86_;
  assign po23 = 1'b1;
endmodule


