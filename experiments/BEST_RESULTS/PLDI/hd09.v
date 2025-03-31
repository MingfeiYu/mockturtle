// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 21:05:41 2025

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
    new_n114_, new_n115_, new_n116_, new_n117_, new_n118_, new_n119_,
    new_n120_, new_n121_, new_n122_, new_n123_, new_n124_, new_n125_,
    new_n126_, new_n127_, new_n128_, new_n129_, new_n130_, new_n131_,
    new_n132_, new_n133_, new_n134_, new_n135_, new_n136_, new_n137_,
    new_n138_, new_n139_, new_n140_, new_n141_, new_n142_, new_n143_,
    new_n144_, new_n145_, new_n146_, new_n147_, new_n148_, new_n149_,
    new_n150_, new_n151_, new_n152_, new_n153_, new_n154_, new_n155_,
    new_n156_, new_n157_, new_n158_, new_n159_, new_n160_;
  assign new_n33_ = ~n13 & ~n16;
  assign new_n34_ = ~n1 & ~n2;
  assign new_n35_ = ~n3 & ~n4;
  assign new_n36_ = ~n5 & ~n6;
  assign new_n37_ = new_n35_ & new_n36_;
  assign new_n38_ = new_n34_ & new_n37_;
  assign new_n39_ = ~n11 & ~n12;
  assign new_n40_ = ~n14 & ~n15;
  assign new_n41_ = new_n39_ & new_n40_;
  assign new_n42_ = ~n7 & ~n8;
  assign new_n43_ = ~n9 & ~n10;
  assign new_n44_ = new_n42_ & new_n43_;
  assign new_n45_ = new_n41_ & new_n44_;
  assign new_n46_ = new_n38_ & new_n45_;
  assign new_n47_ = new_n33_ & new_n46_;
  assign new_n48_ = new_n47_ ^ n17;
  assign new_n49_ = ~n17 & new_n47_;
  assign new_n50_ = ~n18 & new_n49_;
  assign new_n51_ = ~new_n50_ & n19;
  assign new_n52_ = ~n23 & ~n24;
  assign new_n53_ = ~n22 & new_n52_;
  assign new_n54_ = ~n21 & new_n53_;
  assign new_n55_ = ~n20 & new_n54_;
  assign new_n56_ = ~n18 & ~n19;
  assign new_n57_ = ~n17 & new_n56_;
  assign new_n58_ = new_n33_ & new_n57_;
  assign new_n59_ = new_n46_ & new_n58_;
  assign new_n60_ = ~new_n59_ & new_n55_;
  assign new_n61_ = ~new_n51_ & new_n60_;
  assign new_n62_ = ~n32 & n31;
  assign new_n63_ = new_n62_ ^ n32;
  assign new_n64_ = ~n29 & ~n30;
  assign new_n65_ = ~new_n63_ & ~new_n64_;
  assign new_n66_ = new_n65_ ^ new_n63_;
  assign new_n67_ = ~n27 & ~n28;
  assign new_n68_ = ~new_n66_ & new_n67_;
  assign new_n69_ = new_n68_ ^ new_n66_;
  assign new_n70_ = new_n60_ ^ new_n55_;
  assign new_n71_ = ~n25 & ~new_n70_;
  assign new_n72_ = new_n71_ ^ n25;
  assign new_n73_ = new_n72_ ^ n26;
  assign new_n74_ = n26 & new_n72_;
  assign new_n75_ = new_n74_ ^ new_n73_;
  assign new_n76_ = ~n28 & ~new_n75_;
  assign new_n77_ = ~new_n66_ & ~new_n76_;
  assign new_n78_ = new_n77_ ^ new_n69_;
  assign new_n79_ = ~n25 & ~n26;
  assign new_n80_ = ~n26 & new_n70_;
  assign new_n81_ = new_n80_ ^ new_n79_;
  assign new_n82_ = new_n68_ & new_n75_;
  assign new_n83_ = ~new_n74_ & new_n82_;
  assign new_n84_ = ~new_n81_ & new_n83_;
  assign new_n85_ = new_n84_ ^ new_n78_;
  assign new_n86_ = new_n85_ ^ new_n83_;
  assign new_n87_ = new_n86_ ^ new_n78_;
  assign new_n88_ = new_n61_ & new_n87_;
  assign new_n89_ = ~new_n48_ & new_n88_;
  assign new_n90_ = ~n18 & new_n89_;
  assign new_n91_ = ~n25 & new_n54_;
  assign new_n92_ = ~n20 & new_n91_;
  assign new_n93_ = new_n59_ & new_n91_;
  assign new_n94_ = new_n93_ ^ new_n92_;
  assign new_n95_ = new_n83_ & new_n94_;
  assign new_n96_ = ~n19 & new_n49_;
  assign new_n97_ = new_n96_ ^ new_n56_;
  assign new_n98_ = new_n95_ & new_n97_;
  assign new_n99_ = new_n98_ ^ new_n88_;
  assign new_n100_ = ~new_n90_ & new_n99_;
  assign new_n101_ = new_n100_ ^ new_n98_;
  assign new_n102_ = ~new_n89_ & ~new_n99_;
  assign new_n103_ = new_n102_ ^ new_n101_;
  assign new_n104_ = ~new_n88_ & ~new_n98_;
  assign new_n105_ = new_n104_ ^ new_n95_;
  assign new_n106_ = ~n20 & new_n58_;
  assign new_n107_ = new_n46_ & new_n106_;
  assign new_n108_ = new_n53_ & new_n107_;
  assign new_n109_ = new_n108_ ^ new_n54_;
  assign new_n110_ = new_n87_ & new_n109_;
  assign new_n111_ = ~new_n95_ & ~new_n110_;
  assign new_n112_ = new_n88_ & new_n111_;
  assign new_n113_ = new_n112_ ^ new_n95_;
  assign new_n114_ = new_n113_ ^ new_n110_;
  assign new_n115_ = ~n21 & new_n107_;
  assign new_n116_ = new_n115_ ^ n22;
  assign new_n117_ = ~n25 & new_n52_;
  assign new_n118_ = new_n83_ & new_n117_;
  assign new_n119_ = ~new_n116_ & new_n118_;
  assign new_n120_ = new_n111_ & new_n119_;
  assign new_n121_ = ~new_n119_ & new_n110_;
  assign new_n122_ = ~new_n120_ & ~new_n121_;
  assign new_n123_ = ~n25 & ~new_n116_;
  assign new_n124_ = ~new_n110_ & ~new_n123_;
  assign new_n125_ = ~new_n87_ & ~new_n123_;
  assign new_n126_ = ~n22 & new_n115_;
  assign new_n127_ = ~n24 & new_n126_;
  assign new_n128_ = new_n127_ ^ new_n52_;
  assign new_n129_ = ~new_n125_ & new_n128_;
  assign new_n130_ = new_n124_ & new_n129_;
  assign new_n131_ = ~n23 & new_n126_;
  assign new_n132_ = ~new_n131_ & n24;
  assign new_n133_ = new_n71_ & new_n83_;
  assign new_n134_ = ~new_n132_ & new_n133_;
  assign new_n135_ = ~new_n128_ & new_n134_;
  assign new_n136_ = new_n134_ ^ new_n87_;
  assign new_n137_ = new_n83_ ^ new_n78_;
  assign new_n138_ = new_n67_ ^ n28;
  assign new_n139_ = ~n27 & ~new_n75_;
  assign new_n140_ = new_n139_ ^ new_n138_;
  assign new_n141_ = ~new_n140_ & new_n77_;
  assign new_n142_ = new_n139_ ^ n28;
  assign new_n143_ = new_n139_ ^ n29;
  assign new_n144_ = ~new_n143_ & new_n142_;
  assign new_n145_ = ~new_n63_ & n30;
  assign new_n146_ = new_n145_ ^ new_n63_;
  assign new_n147_ = ~new_n146_ & new_n144_;
  assign new_n148_ = ~n28 & new_n139_;
  assign new_n149_ = ~new_n148_ & n30;
  assign new_n150_ = ~n29 & new_n148_;
  assign new_n151_ = new_n150_ ^ new_n148_;
  assign new_n152_ = ~new_n151_ & new_n65_;
  assign new_n153_ = ~new_n149_ & new_n152_;
  assign new_n154_ = ~new_n150_ & new_n145_;
  assign new_n155_ = ~n30 & new_n150_;
  assign new_n156_ = new_n62_ & new_n155_;
  assign new_n157_ = ~new_n154_ & ~new_n156_;
  assign new_n158_ = new_n155_ ^ n32;
  assign new_n159_ = n32 ^ n31;
  assign new_n160_ = ~new_n158_ & new_n159_;
  assign po0 = new_n90_;
  assign po1 = ~new_n103_;
  assign po2 = new_n100_;
  assign po3 = ~new_n105_;
  assign po4 = new_n114_;
  assign po5 = ~new_n122_;
  assign po6 = new_n130_;
  assign po7 = new_n135_;
  assign po8 = new_n136_;
  assign po9 = new_n84_;
  assign po10 = ~new_n137_;
  assign po11 = new_n141_;
  assign po12 = new_n147_;
  assign po13 = new_n153_;
  assign po14 = ~new_n157_;
  assign po15 = new_n160_;
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
  assign po29 = 1'b0;
  assign po30 = 1'b0;
  assign po31 = 1'b0;
endmodule


