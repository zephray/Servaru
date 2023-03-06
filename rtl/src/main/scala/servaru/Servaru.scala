package servaru

import chisel3._

class ServaruTop extends Module {
  val io = IO(new Bundle {
    val in = Input(Bool())
    val out = Output(Bool())
  })

  io.out := io.in
}

object ServaruMain extends App {
  (new chisel3.stage.ChiselStage).emitVerilog(new ServaruTop(), Array("--target-dir", "generated"))
}