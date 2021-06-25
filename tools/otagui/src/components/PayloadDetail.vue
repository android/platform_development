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
        <p
          class="toggle"
          @click="toggle(partition.partitionName)"
        >
          Total Operations: {{ partition.operationsList.length }}
          <ul
            v-if="partitionShowOPs.get(partition.partitionName)"
          >
            <li
              v-for="operation in partition.operationsList"
              :key="operation.dataSha256Hash"
            >
              <OperationDetail
                :operation="operation"
                :mapType="opType.mapType"
              />
            </li>
          </ul>
        </p>
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
import OperationDetail from '@/components/OperationDetail.vue'
import { OpType } from '@/services/payload.js'

export default {
  components: {
    OperationDetail,
  },
  props: {
    zipFile: {
      type: File,
      default: null,
    },
    payload: {
      type: Object,
      default: null,
    },
  },
  data() {
    return {
      partitionShowOPs: new Map(),
      opType: null,
    }
  },
  created() {
    this.opType = new OpType()
  },
  methods: {
    toggle(name) {
      this.partitionShowOPs.set(name, !this.partitionShowOPs.get(name))
    },
  },
}
</script>

<style scoped>
.signature {
  overflow: scroll;
  height: 200px;
  width: 100%;
  word-break: break-all;
}
.toggle {
  display: block;
  cursor: pointer;
  color: #00c255;
}
</style>