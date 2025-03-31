// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 20:44:56 2025

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
    new_n68_, new_n69_, new_n70_, new_n71_, new_n72_, new_n73_, new_n74_,
    new_n75_, new_n76_, new_n77_, new_n78_, new_n79_, new_n80_, new_n81_,
    new_n82_, new_n83_, new_n84_, new_n85_, new_n86_, new_n87_, new_n88_,
    new_n89_, new_n90_, new_n91_, new_n92_, new_n93_, new_n94_, new_n95_,
    new_n96_, new_n97_, new_n98_, new_n99_, new_n100_, new_n101_,
    new_n102_, new_n103_, new_n104_, new_n105_, new_n106_, new_n107_,
    new_n108_, new_n109_, new_n110_, new_n111_, new_n112_, new_n113_,
    new_n114_, new_n115_;
  assign new_n33_ = ~n1 & ~n2;
  assign new_n34_ = new_n33_ ^ n1;
  assign new_n35_ = n3 & new_n33_;
  assign new_n36_ = ~n3 & ~n4;
  assign new_n37_ = new_n33_ & new_n36_;
  assign new_n38_ = new_n37_ ^ new_n33_;
  assign new_n39_ = new_n38_ ^ new_n35_;
  assign new_n40_ = n5 & new_n37_;
  assign new_n41_ = ~n5 & ~n6;
  assign new_n42_ = new_n37_ & new_n41_;
  assign new_n43_ = new_n42_ ^ new_n37_;
  assign new_n44_ = new_n43_ ^ new_n40_;
  assign new_n45_ = n7 & new_n42_;
  assign new_n46_ = ~n7 & ~n8;
  assign new_n47_ = new_n42_ & new_n46_;
  assign new_n48_ = new_n47_ ^ new_n42_;
  assign new_n49_ = new_n48_ ^ new_n45_;
  assign new_n50_ = n9 & new_n47_;
  assign new_n51_ = ~n9 & ~n10;
  assign new_n52_ = new_n47_ & new_n51_;
  assign new_n53_ = new_n52_ ^ new_n47_;
  assign new_n54_ = new_n53_ ^ new_n50_;
  assign new_n55_ = n11 & new_n52_;
  assign new_n56_ = ~n11 & ~n12;
  assign new_n57_ = new_n52_ & new_n56_;
  assign new_n58_ = new_n57_ ^ new_n52_;
  assign new_n59_ = new_n58_ ^ new_n55_;
  assign new_n60_ = n13 & new_n57_;
  assign new_n61_ = ~n13 & ~n14;
  assign new_n62_ = new_n61_ ^ n13;
  assign new_n63_ = ~new_n62_ & new_n57_;
  assign new_n64_ = n15 & new_n61_;
  assign new_n65_ = new_n57_ & new_n64_;
  assign new_n66_ = ~n15 & ~n16;
  assign new_n67_ = new_n61_ & new_n66_;
  assign new_n68_ = new_n67_ ^ new_n61_;
  assign new_n69_ = new_n68_ ^ new_n64_;
  assign new_n70_ = new_n57_ & new_n69_;
  assign new_n71_ = new_n57_ & new_n67_;
  assign new_n72_ = n17 & new_n71_;
  assign new_n73_ = ~n17 & ~n18;
  assign new_n74_ = new_n73_ ^ n17;
  assign new_n75_ = ~new_n74_ & new_n71_;
  assign new_n76_ = ~n19 & new_n73_;
  assign new_n77_ = new_n76_ ^ new_n73_;
  assign new_n78_ = new_n71_ & new_n77_;
  assign new_n79_ = ~n20 & new_n76_;
  assign new_n80_ = new_n79_ ^ new_n76_;
  assign new_n81_ = new_n71_ & new_n80_;
  assign new_n82_ = new_n71_ & new_n79_;
  assign new_n83_ = n21 & new_n82_;
  assign new_n84_ = ~n21 & ~n22;
  assign new_n85_ = new_n84_ ^ n21;
  assign new_n86_ = ~new_n85_ & new_n82_;
  assign new_n87_ = ~n23 & new_n84_;
  assign new_n88_ = new_n87_ ^ new_n84_;
  assign new_n89_ = new_n82_ & new_n88_;
  assign new_n90_ = ~n24 & new_n87_;
  assign new_n91_ = new_n90_ ^ new_n87_;
  assign new_n92_ = new_n82_ & new_n91_;
  assign new_n93_ = new_n82_ & new_n90_;
  assign new_n94_ = n25 & new_n93_;
  assign new_n95_ = ~n25 & ~n26;
  assign new_n96_ = new_n95_ ^ n25;
  assign new_n97_ = ~new_n96_ & new_n93_;
  assign new_n98_ = ~n27 & new_n95_;
  assign new_n99_ = new_n98_ ^ new_n95_;
  assign new_n100_ = new_n93_ & new_n99_;
  assign new_n101_ = ~n28 & new_n98_;
  assign new_n102_ = new_n101_ ^ new_n98_;
  assign new_n103_ = new_n93_ & new_n102_;
  assign new_n104_ = ~n29 & new_n101_;
  assign new_n105_ = new_n104_ ^ new_n101_;
  assign new_n106_ = new_n93_ & new_n105_;
  assign new_n107_ = n30 & new_n104_;
  assign new_n108_ = new_n93_ & new_n107_;
  assign new_n109_ = ~n30 & ~n31;
  assign new_n110_ = new_n109_ ^ n30;
  assign new_n111_ = ~new_n110_ & new_n104_;
  assign new_n112_ = new_n93_ & new_n111_;
  assign new_n113_ = n32 & new_n109_;
  assign new_n114_ = new_n104_ & new_n113_;
  assign new_n115_ = new_n93_ & new_n114_;
  assign po0 = n1;
  assign po1 = ~new_n34_;
  assign po2 = new_n35_;
  assign po3 = new_n39_;
  assign po4 = new_n40_;
  assign po5 = new_n44_;
  assign po6 = new_n45_;
  assign po7 = new_n49_;
  assign po8 = new_n50_;
  assign po9 = new_n54_;
  assign po10 = new_n55_;
  assign po11 = new_n59_;
  assign po12 = new_n60_;
  assign po13 = new_n63_;
  assign po14 = new_n65_;
  assign po15 = new_n70_;
  assign po16 = new_n72_;
  assign po17 = new_n75_;
  assign po18 = new_n78_;
  assign po19 = new_n81_;
  assign po20 = new_n83_;
  assign po21 = new_n86_;
  assign po22 = new_n89_;
  assign po23 = new_n92_;
  assign po24 = new_n94_;
  assign po25 = new_n97_;
  assign po26 = new_n100_;
  assign po27 = new_n103_;
  assign po28 = new_n106_;
  assign po29 = new_n108_;
  assign po30 = new_n112_;
  assign po31 = new_n115_;
endmodule


