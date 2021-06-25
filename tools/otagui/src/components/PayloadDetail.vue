<template>
  <div v-if="zipFile">
    <h3>File infos</h3>
    <ul>
      <li>File name: {{ zipFile.name }}</li>
      <li>File size: {{ zipFile.size }} Bytes</li>
      <li>File last modified date: {{ zipFile.lastModifiedDate }}</li>
    </ul>
  </div>
  <div v-if="payload">
    <h3>Partition List</h3>
    <ul v-if="payload.manifest">
      <li
        v-for="partition in payload.manifest.partitionsList"
        :key="partition.partitionName"
      >
        <h4>{{ partition.partitionName }}</h4>
        <p v-if="partition.estimateCowSize">
          Estimate COW Size: {{ partition.estimateCowSize }} Bytes
        </p>
        <p v-else>
          Estimate COW Size: 0 Bytes
        </p>
        <PartitionDetail 
          :partition="partition"
        />
      </li>
    </ul>
    <h3>Metadata Signature</h3>
    <div
      v-if="payload.metadata_signature"
      class="signature"
    >
      {{ payload.metadata_signature.signaturesList[0].data }}
    </div>
  </div>
</template>

<script>
import PartitionDetail from './PartitionDetail.vue'

export default {
  components: {
    PartitionDetail
  },
  props: {
    zipFile: {
      type: Object,
      default: null
    },
    payload: {
      type: Object,
      default: null,
    },
  }
}
</script>

<style scoped>
.signature {
  overflow: scroll;
  height: 200px;
  width: 100%;
  word-break: break-all;
}
</style>