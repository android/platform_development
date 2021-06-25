<template>
  {{ mapType.get(operation.type) }}
  <p v-if="operation.dataOffset !== null">
    Data offset: {{ operation.dataOffset }}
  </p>
  <p v-if="operation.dataLength !== null">
    Data length: {{ operation.dataLength }}
  </p>
  <p v-if="operation.srcExtents !== null">
    Source: {{ operation.srcExtents.length }} extents ({{ srcTotalBlocks }}
    blocks)
    <br>
    {{ srcBlocks }}
  </p>
  <p v-if="operation.dstExtents !== null">
    Destination: {{ operation.dstExtents.length }} extents ({{ dstTotalBlocks }}
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
  mounted() {
    if (this.operation.srcExtents) {
      this.srcTotalBlocks = this.numBlocks(this.operation.srcExtents)
      this.srcBlocks = this.displayBlocks(this.operation.srcExtents)
    }
    if (this.operation.dstExtents) {
      this.dstTotalBlocks = this.numBlocks(this.operation.dstExtents)
      this.dstBlocks = this.displayBlocks(this.operation.dstExtents)
    }
  },
  methods: {
    numBlocks(exts) {
      const accumulator = (total, ext) => total + ext['numBlocks']
      return exts.reduce(accumulator, 0)
    },
    displayBlocks(exts) {
      const accumulator = (total, ext) =>
        total + '(' + ext['startBlock'] + ',' + ext['numBlocks'] + ')'
      return exts.reduce(accumulator, '')
    },
  },
}
</script>