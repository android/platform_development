<template>
  {{ mapType.get(operation.type) }}
  <p v-if="operation.dataOffset != null">
    Data offset: {{ operation.dataOffset }}
  </p>
  <p v-if="operation.dataLength != null">
    Data length: {{ operation.dataLength }}
  </p>
  <p v-if="operation.srcExtentsList != null">
    Source: {{ operation.srcExtentsList.length }} extents ({{ srcTotalBlocks }}
    blocks)
    <br>
    {{ srcBlocks }}
  </p>
  <p v-if="operation.dstExtentsList != null">
    Destination: {{ operation.dstExtentsList.length }} extents ({{
      dstTotalBlocks
    }}
    blocks)
    <br>
    {{ dstBlocks }}
  </p>
</template>

<script>
export default {
  props: {
    operation: {
      type: Object,
      required: true,
    },
    mapType: {
      type: Map,
      required: true,
    },
  },
  data() {
    return {
      srcTotalBlocks: null,
      srcBlocks: null,
      dstTotalBlocks: null,
      dstBlocks: null,
    }
  },
  async mounted() {
    if (this.operation.srcExtentsList) {
      this.srcTotalBlocks = await this.numBlocks(this.operation.srcExtentsList)
      this.srcBlocks = await this.displayBlocks(this.operation.srcExtentsList)
    }
    if (this.operation.dstExtentsList) {
      this.dstTotalBlocks = await this.numBlocks(this.operation.dstExtentsList)
      this.dstBlocks = await this.displayBlocks(this.operation.dstExtentsList)
    }
  },
  methods: {
    async numBlocks(exts) {
      const accumulator = async (total, ext) => 
        await total + ext['numBlocks']
      return await exts.reduce(accumulator, 0)
    },
    async displayBlocks(exts) {
      const accumulator = async (total, ext) => 
        await total + '(' + ext['startBlock'] + ',' 
        + ext['numBlocks'] + ')'
      return await exts.reduce(accumulator, '')
    },
  },
}
</script>