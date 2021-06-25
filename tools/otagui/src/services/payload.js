/**
 * @fileoverview Clss paypload is used to read in and
 * parse the payload.bin file from a OTA.zip file.
 * Class OpType creates a Map that can resolve the
 * operation type.
 * @package zip.js
 * @package google-protobuf
 */

import * as zip from '@zip.js/zip.js/dist/zip-full.min.js'
let update_metadata_pb = require('./update_metadata_pb')

const _MAGIC = 'CrAU'
const _VERSION_SIZE = 8
const _MANIFEST_LEN_SIZE = 8
const _METADATA_SIGNATURE_LEN_SIZE = 4
const _BRILLO_MAJOR_PAYLOAD_VERSION = 2

export class Payload {
  /**
   * This class parses the metadata of a OTA package.
   * @param {File} file A OTA.zip file read from user's machine.
   */
  constructor(file) {
    this.packedFile = new zip.ZipReader(new zip.BlobReader(file))
    this.cursor = 0
  }

  async unzipPayload() {
    let entries = await this.packedFile.getEntries()
    this.payload = null
    for (let entry of entries) {
      if (entry.filename == 'payload.bin') {
        this.payload = await entry.getData(new zip.BlobWriter())
      }
    }
    if (!this.payload) {
      alert('Please select a legit OTA package')
      return
    }
    this.buffer = await this.payload.arrayBuffer()
  }

  /**
   * Read in an integer from binary bufferArray.
   * @param {Int} size the size of a integer being read in
   * @return {Int} an integer.
   */
  async readInt(size) {
    let view = new DataView(
      await this.buffer.slice(this.cursor, this.cursor + size))
    this.cursor += size
    switch (size) {
    case 2:
      return view.getUInt16(0)
    case 4:
      return view.getUint32(0)
    case 8:
      return Number(view.getBigUint64(0))
    default:
      throw 'Cannot read this integer with size ' + size
    }
  }

  async readHeader() {
    let decoder = new TextDecoder()
    try {
      this.magic = decoder.decode(
        this.buffer.slice(this.cursor, _MAGIC.length))
      this.cursor += _MAGIC.length
      if (this.magic != _MAGIC) {
        alert('MAGIC is not correct, please double check.')
      }
      this.header_version = await this.readInt(_VERSION_SIZE)
      this.manifest_len = await this.readInt(_MANIFEST_LEN_SIZE)
      if (this.header_version == _BRILLO_MAJOR_PAYLOAD_VERSION) {
        this.metadata_signature_len = await this.readInt(_METADATA_SIGNATURE_LEN_SIZE)
      }
    } catch (err) {
      console.log(err)
      return
    }
  }

  /**
   * Read in the manifest in an OTA.zip file.
   * The structure of the manifest can be found in:
   * aosp/system/update_engine/update_metadata.proto
   */
  async readManifest() {
    let manifest_raw = await this.buffer.slice(
      this.cursor, this.cursor + this.manifest_len
    )
    this.cursor += this.manifest_len
    this.manifest = await update_metadata_pb.DeltaArchiveManifest
      .deserializeBinary(
        manifest_raw
      ).toObject()
  }

  async readSignature() {
    let signature_raw = await this.buffer.slice(
      this.cursor, this.cursor + this.metadata_signature_len
    )
    this.cursor += this.metadata_signature_len
    this.metadata_signature = await update_metadata_pb.Signatures
      .deserializeBinary(
        signature_raw
      ).toObject()
  }

  async init() {
    await this.unzipPayload()
    await this.readHeader()
    await this.readManifest()
    await this.readSignature()
  }

}

export class OpType {
  /**
   * OpType.mapType create a map that could resolve the operation
   * types. The operation types are encoded as numbers in
   * update_metadata.proto and must be decoded before any usage.
   */
  constructor() {
    let types = update_metadata_pb.InstallOperation.Type
    this.mapType = new Map()
    for (let key in types) {
      this.mapType.set(types[key], key)
    }
  }
}